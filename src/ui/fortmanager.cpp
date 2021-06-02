#include "fortmanager.h"

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QStyle>
#include <QStyleFactory>
#include <QThreadPool>
#include <QWindow>

#include <fort_version.h>

#include "appinfo/appinfocache.h"
#include "appinfo/appinfomanager.h"
#include "conf/firewallconf.h"
#include "control/controlmanager.h"
#include "form/controls/mainwindow.h"
#include "form/dialog/passworddialog.h"
#include "form/graph/graphwindow.h"
#include "form/opt/optionswindow.h"
#include "form/prog/programswindow.h"
#include "form/stat/statisticswindow.h"
#include "form/tray/trayicon.h"
#include "form/zone/zoneswindow.h"
#include "fortcompat.h"
#include "fortsettings.h"
#include "model/applistmodel.h"
#include "model/zonelistmodel.h"
#include "rpc/appinfomanagerrpc.h"
#include "rpc/confmanagerrpc.h"
#include "rpc/drivermanagerrpc.h"
#include "rpc/logmanagerrpc.h"
#include "rpc/quotamanagerrpc.h"
#include "rpc/rpcmanager.h"
#include "rpc/statmanagerrpc.h"
#include "rpc/taskmanagerrpc.h"
#include "task/taskinfozonedownloader.h"
#include "translationmanager.h"
#include "user/usersettings.h"
#include "util/dateutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/hotkeymanager.h"
#include "util/ioc/ioccontainer.h"
#include "util/logger.h"
#include "util/nativeeventfilter.h"
#include "util/net/hostinfocache.h"
#include "util/osutil.h"
#include "util/startuputil.h"

namespace {

void setupAppStyle()
{
    const auto fusionStyle = QStyleFactory::create("Fusion");
    QApplication::setStyle(fusionStyle);
    QApplication::setPalette(fusionStyle->standardPalette());
}

}

FortManager::FortManager(QObject *parent) : QObject(parent), m_initialized(false) { }

FortManager::~FortManager()
{
    if (m_initialized) {
        closeMainWindow();

        closeDriver();
    }

    deleteManagers();

    OsUtil::closeMutex(m_instanceMutex);
}

bool FortManager::checkRunningInstance()
{
    bool isSingleInstance;
    m_instanceMutex = OsUtil::createMutex(
            IoC<FortSettings>()->isService() ? "Global\\" APP_BASE : APP_BASE, isSingleInstance);

    if (!isSingleInstance) {
        showErrorBox(tr("Application is already running!"));
    }
    return isSingleInstance;
}

void FortManager::initialize()
{
    m_initialized = true;

    setupThreadPool();
    setupLogger();

    createManagers();

    setupEnvManager();
    setupConfManager();
    setupQuotaManager();
    setupTaskManager();
    setupModels();

    setupDriver();
    loadConf();
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(8, QThread::idealThreadCount() * 2));
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    const auto settings = IoC<FortSettings>();

    logger->setIsService(settings->isService());
    logger->setPath(settings->logsPath());
}

void FortManager::updateLogger()
{
    const FirewallConf *conf = IoC<ConfManager>()->conf();

    if (!conf->iniEdited())
        return;

    Logger *logger = Logger::instance();

    logger->setDebug(conf->ini().logDebug());
    logger->setConsole(conf->ini().logConsole());
}

void FortManager::createManagers()
{
    IocContainer *ioc = IoC();

    const auto settings = IoC<FortSettings>();

    if (settings->hasService()) {
        ioc->setService(new RpcManager());
    }

    ConfManager *confManager;
    QuotaManager *quotaManager;
    StatManager *statManager;
    DriverManager *driverManager;
    AppInfoManager *appInfoManager;
    LogManager *logManager;
    TaskManager *taskManager;

    if (settings->isMaster()) {
        confManager = new ConfManager(settings->confFilePath());
        quotaManager = new QuotaManager();
        statManager = new StatManager(settings->statFilePath());
        driverManager = new DriverManager();
        appInfoManager = new AppInfoManager(settings->cacheFilePath());
        logManager = new LogManager();
        taskManager = new TaskManager();
    } else {
        confManager = new ConfManagerRpc(settings->confFilePath());
        quotaManager = new QuotaManagerRpc();
        statManager = new StatManagerRpc(settings->statFilePath());
        driverManager = new DriverManagerRpc();
        appInfoManager = new AppInfoManagerRpc(settings->cacheFilePath());
        logManager = new LogManagerRpc();
        taskManager = new TaskManagerRpc();
    }

    ioc->setService<ConfManager>(confManager);
    ioc->setService<QuotaManager>(quotaManager);
    ioc->setService<StatManager>(statManager);
    ioc->setService<DriverManager>(driverManager);
    ioc->setService<AppInfoManager>(appInfoManager);
    ioc->setService<LogManager>(logManager);
    ioc->setService<TaskManager>(taskManager);

    ioc->setService(new AppInfoCache());
    ioc->setService(new HostInfoCache());

    ioc->setService(new NativeEventFilter());
    ioc->setService(new HotKeyManager());

    if (!settings->isService()) {
        ioc->setService(new UserSettings());
        ioc->setService(new TranslationManager());
    }

    ioc->setUpAll();
}

void FortManager::deleteManagers()
{
    IocContainer *ioc = IoC();

    ioc->tearDownAll();
    ioc->autoDeleteAll();
}

bool FortManager::installDriver()
{
    closeDriver();

    IoC<DriverManager>()->reinstallDriver();

    if (IoC<FortSettings>()->hasService()) {
        // Re-install the service and app restart required to continue
        StartupUtil::setServiceInstalled(true);
        processRestartRequired();
    } else {
        // Re-open the driver device and initialize it
        if (setupDriver()) {
            updateDriverConf();
        }
    }

    return true;
}

bool FortManager::removeDriver()
{
    closeDriver();

    IoC<DriverManager>()->uninstallDriver();

    return true;
}

bool FortManager::setupDriver()
{
    auto driverManager = IoC<DriverManager>();

    bool ok = driverManager->openDevice();

    if (ok && !IoC<ConfManager>()->validateDriver()) {
        driverManager->closeDevice();
        ok = false;
    }

    return ok;
}

void FortManager::closeDriver()
{
    updateLogManager(false);
    updateStatManager(nullptr);

    IoC<DriverManager>()->closeDevice();

    QCoreApplication::sendPostedEvents(this);
}

void FortManager::setupEnvManager()
{
    auto envManager = IoC<EnvManager>();

    connect(IoC<NativeEventFilter>(), &NativeEventFilter::environmentChanged, envManager,
            &EnvManager::onEnvironmentChanged);

    connect(envManager, &EnvManager::environmentUpdated, this, [&] { updateDriverConf(); });
}

void FortManager::setupConfManager()
{
    connect(IoC<ConfManager>(), &ConfManager::confChanged, this, [&](bool onlyFlags) {
        updateLogger();

        if (!onlyFlags || IoC<ConfManager>()->conf()->flagsEdited()) {
            updateDriverConf(onlyFlags);
        }
    });
}

void FortManager::setupQuotaManager()
{
    if (!IoC<FortSettings>()->isService()) {
        connect(IoC<QuotaManager>(), &QuotaManager::alert, this, [&](qint8 alertType) {
            showInfoBox(QuotaManager::alertTypeText(alertType), tr("Quota Alert"));
        });
    }
}

void FortManager::setupTaskManager()
{
    auto taskManager = IoC<TaskManager>();

    connect(taskManager, &TaskManager::taskDoubleClicked, this, [&](qint8 taskType) {
        if (taskType == TaskInfo::ZoneDownloader) {
            showZonesWindow();
        }
    });
    connect(taskManager->taskInfoZoneDownloader(), &TaskInfoZoneDownloader::zonesUpdated,
            IoC<ConfManager>(), &ConfManager::updateDriverZones);
}

void FortManager::setupModels()
{
    m_appListModel = new AppListModel(this);
    appListModel()->initialize();

    m_zoneListModel = new ZoneListModel(this);
    zoneListModel()->initialize();
}

void FortManager::setupTranslationManager()
{
    IoC<TranslationManager>()->switchLanguageByName(IoC<UserSettings>()->iniUser().language());
}

void FortManager::setupMainWindow()
{
    m_mainWindow = new MainWindow();

    auto nativeEventFilter = IoC<NativeEventFilter>();

    nativeEventFilter->registerSessionNotification(m_mainWindow->winId());

    connect(nativeEventFilter, &NativeEventFilter::sessionLocked, this, [&] {
        IoC<FortSettings>()->resetCheckedPassword(PasswordDialog::UnlockTillSessionLock);
    });
}

void FortManager::closeMainWindow()
{
    if (!m_mainWindow)
        return;

    auto nativeEventFilter = IoC<NativeEventFilter>();

    nativeEventFilter->unregisterHotKeys();
    nativeEventFilter->unregisterSessionNotification(m_mainWindow->winId());

    delete m_mainWindow;
}

void FortManager::setupTrayIcon()
{
    m_trayIcon = new TrayIcon(this);

    connect(m_trayIcon, &TrayIcon::mouseClicked, this, &FortManager::showProgramsWindow);
    connect(m_trayIcon, &TrayIcon::mouseDoubleClicked, this, &FortManager::showOptionsWindow);
    connect(m_trayIcon, &TrayIcon::mouseMiddleClicked, this, &FortManager::showStatisticsWindow);
    connect(m_trayIcon, &TrayIcon::mouseRightClicked, m_trayIcon, &TrayIcon::showTrayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &FortManager::onTrayMessageClicked);

    auto confManager = IoC<ConfManager>();
    connect(confManager, &ConfManager::confChanged, m_trayIcon, &TrayIcon::updateTrayMenu);
    connect(confManager, &ConfManager::iniUserChanged, m_trayIcon, &TrayIcon::updateTrayMenu);
    connect(confManager, &ConfManager::appAlerted, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(true); });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &FortManager::closeUi);
}

void FortManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow();
    m_progWindow->restoreWindowState();

    connect(m_progWindow, &ProgramsWindow::aboutToClose, this, &FortManager::closeProgramsWindow);
    connect(m_progWindow, &ProgramsWindow::activationChanged, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(false); });
}

void FortManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow();
    m_optWindow->restoreWindowState();

    connect(m_optWindow, &OptionsWindow::aboutToClose, this, &FortManager::closeOptionsWindow);
}

void FortManager::setupZonesWindow()
{
    m_zoneWindow = new ZonesWindow();
    m_zoneWindow->restoreWindowState();

    connect(m_zoneWindow, &ZonesWindow::aboutToClose, this, &FortManager::closeZonesWindow);
}

void FortManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow();
    m_graphWindow->restoreWindowState();

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, this,
            [&](QMouseEvent *event) { m_trayIcon->showTrayMenu(mouseEventGlobalPos(event)); });

    connect(IoC<StatManager>(), &StatManager::trafficAdded, m_graphWindow,
            &GraphWindow::addTraffic);
}

void FortManager::setupStatisticsWindow()
{
    m_statWindow = new StatisticsWindow();
    m_statWindow->restoreWindowState();

    connect(m_statWindow, &StatisticsWindow::aboutToClose, this,
            &FortManager::closeStatisticsWindow);
}

void FortManager::closeUi()
{
    closeGraphWindow(true);
    closeOptionsWindow();
    closeProgramsWindow();
    closeZonesWindow();
    closeStatisticsWindow();
    closeTrayIcon();
}

void FortManager::show()
{
    setupAppStyle(); // Style & Palette

    showTrayIcon();

    if (IoC<UserSettings>()->iniUser().graphWindowVisible()) {
        showGraphWindow();
    }
}

void FortManager::showTrayIcon()
{
    if (!m_trayIcon) {
        setupTranslationManager();
        setupMainWindow();
        setupTrayIcon();
    }

    m_trayIcon->show();
}

void FortManager::closeTrayIcon()
{
    m_trayIcon->hide();
}

void FortManager::showTrayMessage(const QString &message, FortManager::TrayMessageType type)
{
    if (!m_trayIcon)
        return;

    m_lastMessageType = type;
    m_trayIcon->showMessage(QGuiApplication::applicationDisplayName(), message);
}

void FortManager::showProgramsWindow()
{
    if (!(m_progWindow && m_progWindow->isVisible()) && !checkPassword())
        return;

    if (!m_progWindow) {
        setupProgramsWindow();
    }

    m_progWindow->show();
    m_progWindow->raise();
    m_progWindow->activateWindow();
}

void FortManager::closeProgramsWindow()
{
    if (!m_progWindow)
        return;

    m_progWindow->saveWindowState();
    m_progWindow->hide();

    m_progWindow->deleteLater();
    m_progWindow = nullptr;
}

bool FortManager::showProgramEditForm(const QString &appPath)
{
    showProgramsWindow();
    if (!(m_progWindow && m_progWindow->isVisible()))
        return false; // May be not opened due to password checking

    if (!m_progWindow->editProgramByPath(appPath)) {
        showErrorBox(tr("Please close already opened Edit Program window and try again."));
        return false;
    }
    return true;
}

void FortManager::showOptionsWindow()
{
    if (!(m_optWindow && m_optWindow->isVisible()) && !checkPassword())
        return;

    if (!m_optWindow) {
        setupOptionsWindow();

        emit optWindowChanged(true);
    }

    m_optWindow->show();
    m_optWindow->raise();
    m_optWindow->activateWindow();
}

void FortManager::closeOptionsWindow()
{
    if (!m_optWindow)
        return;

    m_optWindow->cancelChanges();
    m_optWindow->saveWindowState();
    m_optWindow->hide();

    m_optWindow->deleteLater();
    m_optWindow = nullptr;

    emit optWindowChanged(false);
}

void FortManager::reloadOptionsWindow(const QString &reason)
{
    if (!m_optWindow)
        return;

    // Unsaved changes are lost
    closeOptionsWindow();
    showOptionsWindow();

    showTrayMessage(reason);
}

void FortManager::showStatisticsWindow()
{
    if (!(m_statWindow && m_statWindow->isVisible()) && !checkPassword())
        return;

    if (!m_statWindow) {
        setupStatisticsWindow();
    }

    m_statWindow->show();
    m_statWindow->raise();
    m_statWindow->activateWindow();
}

void FortManager::closeStatisticsWindow()
{
    if (!m_statWindow)
        return;

    m_statWindow->saveWindowState();
    m_statWindow->hide();

    m_statWindow->deleteLater();
    m_statWindow = nullptr;
}

void FortManager::showZonesWindow()
{
    if (!(m_zoneWindow && m_zoneWindow->isVisible()) && !checkPassword())
        return;

    if (!m_zoneWindow) {
        setupZonesWindow();
    }

    m_zoneWindow->show();
    m_zoneWindow->raise();
    m_zoneWindow->activateWindow();
}

void FortManager::closeZonesWindow()
{
    if (!m_zoneWindow)
        return;

    m_zoneWindow->saveWindowState();
    m_zoneWindow->hide();

    m_zoneWindow->deleteLater();
    m_zoneWindow = nullptr;
}

void FortManager::showGraphWindow()
{
    if (!m_graphWindow) {
        setupGraphWindow();

        emit graphWindowChanged(true);
    }

    m_graphWindow->show();
}

void FortManager::closeGraphWindow(bool wasVisible)
{
    if (!m_graphWindow)
        return;

    m_graphWindow->saveWindowState(wasVisible);
    m_graphWindow->hide();

    m_graphWindow->deleteLater();
    m_graphWindow = nullptr;

    emit graphWindowChanged(false);
}

void FortManager::switchGraphWindow()
{
    if (!m_graphWindow)
        showGraphWindow();
    else
        closeGraphWindow();
}

void FortManager::processRestartRequired()
{
    if (!showYesNoBox(tr("Restart Required"), tr("Restart Now"), tr("Later")))
        return;

    const QString appFilePath = QCoreApplication::applicationFilePath();
    const QStringList args = IoC<FortSettings>()->appArguments();

    connect(qApp, &QObject::destroyed, [=] { QProcess::startDetached(appFilePath, args); });

    qDebug() << "Quit due required restart";

    QCoreApplication::quit();
}

void FortManager::quitByCheckPassword()
{
    if (!checkPassword())
        return;

    closeUi();

    qDebug() << "Quit due user request";

    QCoreApplication::quit();
}

bool FortManager::checkPassword()
{
    static bool g_passwordDialogOpened = false;

    const auto settings = IoC<FortSettings>();

    if (!settings->isPasswordRequired())
        return true;

    if (g_passwordDialogOpened) {
        activateModalWidget();
        return false;
    }

    g_passwordDialogOpened = true;

    QString password;
    PasswordDialog::UnlockType unlockType = PasswordDialog::UnlockDisabled;
    const bool ok = PasswordDialog::getPassword(password, unlockType, m_mainWindow);

    g_passwordDialogOpened = false;

    const bool checked = ok && !password.isEmpty() && IoC<ConfManager>()->checkPassword(password);

    settings->setPasswordChecked(checked, unlockType);

    return checked;
}

void FortManager::showErrorBox(const QString &text, const QString &title)
{
    QMessageBox::warning(focusWidget(), title, text);
}

void FortManager::showInfoBox(const QString &text, const QString &title)
{
    QMessageBox::information(focusWidget(), title, text);
}

bool FortManager::showQuestionBox(const QString &text, const QString &title)
{
    return QMessageBox::question(focusWidget(), title, text) == QMessageBox::Yes;
}

bool FortManager::showYesNoBox(
        const QString &text, const QString &yesText, const QString &noText, const QString &title)
{
    QMessageBox box(QMessageBox::Information, title, text, QMessageBox::NoButton, focusWidget());
    box.addButton(noText, QMessageBox::NoRole);
    box.addButton(yesText, QMessageBox::YesRole);

    return box.exec() == 1;
}

void FortManager::loadConf()
{
    const auto settings = IoC<FortSettings>();

    QString viaVersion;
    if (!settings->canMigrate(viaVersion)) {
        showInfoBox(tr("Please first install Fort Firewall v%1 and save Options from it.")
                            .arg(viaVersion));
        abort(); // Abort the program
    }

    IoC<ConfManager>()->load();

    qDebug() << "Started as"
             << (settings->isService()                   ? "Service"
                                : settings->hasService() ? "Service Client"
                                                         : "Program");
}

bool FortManager::updateDriverConf(bool onlyFlags)
{
    auto confManager = IoC<ConfManager>();

    updateLogManager(false);

    const bool res = confManager->updateDriverConf(onlyFlags);
    if (res) {
        updateStatManager(confManager->conf());
    }

    updateLogManager(true);

    return res;
}

void FortManager::updateLogManager(bool active)
{
    IoC<LogManager>()->setActive(active);
}

void FortManager::updateStatManager(FirewallConf *conf)
{
    IoC<StatManager>()->setConf(conf);
}

void FortManager::onTrayMessageClicked()
{
    switch (m_lastMessageType) {
    case MessageZones:
        showZonesWindow();
        break;
    default:
        showOptionsWindow();
    }
}

QWidget *FortManager::focusWidget() const
{
    auto w = QApplication::focusWidget();
    return w ? w : m_mainWindow;
}

void FortManager::activateModalWidget()
{
    auto w = qApp->activeModalWidget();
    if (w) {
        w->activateWindow();
    }
}

void FortManager::setupResources()
{
    Q_INIT_RESOURCE(appinfo_migrations);
    Q_INIT_RESOURCE(conf_migrations);
    Q_INIT_RESOURCE(conf_zone);
    Q_INIT_RESOURCE(stat_migrations);

    Q_INIT_RESOURCE(fort_icons);
}
