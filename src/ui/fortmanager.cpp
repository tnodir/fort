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
#include "model/appstatmodel.h"
#include "model/connlistmodel.h"
#include "model/traflistmodel.h"
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

FortManager::FortManager(FortSettings *settings, EnvManager *envManager,
        ControlManager *controlManager, QObject *parent) :
    QObject(parent),
    m_initialized(false),
    m_settings(settings),
    m_envManager(envManager),
    m_controlManager(controlManager)
{
}

FortManager::~FortManager()
{
    if (m_initialized) {
        closeMainWindow();

        closeDriver();
        closeLogManager();
    }

    OsUtil::closeMutex(m_instanceMutex);
}

IniUser *FortManager::iniUser() const
{
    return &userSettings()->iniUser();
}

FirewallConf *FortManager::conf() const
{
    return confManager()->conf();
}

bool FortManager::checkRunningInstance()
{
    bool isSingleInstance;
    m_instanceMutex = OsUtil::createMutex(
            settings()->isService() ? "Global\\" APP_BASE : APP_BASE, isSingleInstance);

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

    setupControlManager();
    setupRpcManager();

    setupEventFilter();
    setupEnvManager();

    setupConfManager();
    setupQuotaManager();
    setupStatManager();
    setupAppInfoManager();

    setupLogManager();
    setupDriverManager();

    setupAppInfoCache();
    setupHostInfoCache();
    setupModels();

    setupTaskManager();

    loadConf();

    qDebug() << "Started as"
             << (settings()->isService()                   ? "Service"
                                : settings()->hasService() ? "Service Client"
                                                           : "Program");
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(8, QThread::idealThreadCount() * 2));
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    logger->setIsService(settings()->isService());
    logger->setPath(settings()->logsPath());
}

void FortManager::updateLogger()
{
    if (!conf()->iniEdited())
        return;

    Logger *logger = Logger::instance();

    logger->setDebug(conf()->ini().logDebug());
    logger->setConsole(conf()->ini().logConsole());
}

void FortManager::createManagers()
{
    if (settings()->hasService()) {
        m_rpcManager = new RpcManager(this, this);
    }

    if (settings()->isMaster()) {
        m_confManager = new ConfManager(settings()->confFilePath(), this, this);
        m_quotaManager = new QuotaManager(confManager(), this);
        m_statManager = new StatManager(settings()->statFilePath(), quotaManager(), this);
        m_driverManager = new DriverManager(this);
        m_appInfoManager = new AppInfoManager(settings()->cacheFilePath(), this);
        m_logManager = new LogManager(this, this);
        m_taskManager = new TaskManager(this, this);
    } else {
        m_confManager = new ConfManagerRpc(settings()->confFilePath(), this, this);
        m_quotaManager = new QuotaManagerRpc(this, this);
        m_statManager = new StatManagerRpc(settings()->statFilePath(), this, this);
        m_driverManager = new DriverManagerRpc(this);
        m_appInfoManager = new AppInfoManagerRpc(settings()->cacheFilePath(), this, this);
        m_logManager = new LogManagerRpc(this, this);
        m_taskManager = new TaskManagerRpc(this, this);
    }
}

void FortManager::setupControlManager()
{
    // Process control commands from clients
    controlManager()->listen(this);
}

void FortManager::setupRpcManager()
{
    if (rpcManager()) {
        rpcManager()->initialize();
    }
}

void FortManager::setupLogManager()
{
    logManager()->initialize();
}

void FortManager::closeLogManager()
{
    logManager()->close();
}

bool FortManager::installDriver()
{
    closeDriver();

    driverManager()->reinstallDriver();

    if (settings()->hasService()) {
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

    driverManager()->uninstallDriver();

    return true;
}

void FortManager::setupDriverManager()
{
    driverManager()->initialize();

    setupDriver();
}

bool FortManager::setupDriver()
{
    bool ok = driverManager()->openDevice();

    if (ok && !confManager()->validateDriver()) {
        driverManager()->closeDevice();
        ok = false;
    }

    return ok;
}

void FortManager::closeDriver()
{
    updateLogManager(false);
    updateStatManager(nullptr);

    driverManager()->closeDevice();
}

void FortManager::setupEventFilter()
{
    m_nativeEventFilter = new NativeEventFilter(this);
}

void FortManager::setupEnvManager()
{
    connect(m_nativeEventFilter, &NativeEventFilter::environmentChanged, envManager(),
            &EnvManager::onEnvironmentChanged);

    connect(envManager(), &EnvManager::environmentUpdated, this, [&] { updateDriverConf(); });
}

void FortManager::setupConfManager()
{
    confManager()->initialize();

    connect(confManager(), &ConfManager::confChanged, this, [&](bool onlyFlags) {
        updateLogger();

        if (!onlyFlags || conf()->flagsEdited()) {
            updateDriverConf(onlyFlags);
        }
    });
}

void FortManager::setupQuotaManager()
{
    if (!settings()->isService()) {
        connect(quotaManager(), &QuotaManager::alert, this, [&](qint8 alertType) {
            showInfoBox(QuotaManager::alertTypeText(alertType), tr("Quota Alert"));
        });
    }
}

void FortManager::setupStatManager()
{
    statManager()->initialize();
}

void FortManager::setupAppInfoManager()
{
    appInfoManager()->initialize();
}

void FortManager::setupAppInfoCache()
{
    m_appInfoCache = new AppInfoCache(this);
    m_appInfoCache->setManager(appInfoManager());
}

void FortManager::setupHostInfoCache()
{
    m_hostInfoCache = new HostInfoCache(this);
}

void FortManager::setupModels()
{
    m_appListModel = new AppListModel(confManager(), this);
    appListModel()->setAppInfoCache(appInfoCache());
    appListModel()->initialize();

    m_appStatModel = new AppStatModel(statManager(), this);
    appStatModel()->setAppInfoCache(appInfoCache());
    appStatModel()->initialize();

    m_zoneListModel = new ZoneListModel(confManager(), this);
    zoneListModel()->initialize();

    m_connListModel = new ConnListModel(statManager(), this);
    connListModel()->setAppInfoCache(appInfoCache());
    connListModel()->setHostInfoCache(hostInfoCache());
}

void FortManager::setupTaskManager()
{
    connect(taskManager(), &TaskManager::taskDoubleClicked, this, [&](qint8 taskType) {
        if (taskType == TaskInfo::ZoneDownloader) {
            showZonesWindow();
        }
    });
    connect(taskManager()->taskInfoZoneDownloader(), &TaskInfoZoneDownloader::zonesUpdated,
            confManager(), &ConfManager::updateDriverZones);

    taskManager()->initialize();
}

void FortManager::setupUserSettings()
{
    m_userSettings = new UserSettings(this);
    m_userSettings->initialize(settings());
}

void FortManager::setupTranslationManager()
{
    TranslationManager::instance()->switchLanguageByName(iniUser()->language());
}

void FortManager::setupMainWindow()
{
    m_mainWindow = new MainWindow();

    m_nativeEventFilter->registerSessionNotification(m_mainWindow->winId());

    connect(m_nativeEventFilter, &NativeEventFilter::sessionLocked, this,
            [&] { settings()->resetCheckedPassword(PasswordDialog::UnlockTillSessionLock); });
}

void FortManager::closeMainWindow()
{
    if (!m_mainWindow)
        return;

    m_nativeEventFilter->unregisterHotKeys();
    m_nativeEventFilter->unregisterSessionNotification(m_mainWindow->winId());

    delete m_mainWindow;
}

void FortManager::setupHotKeyManager()
{
    m_hotKeyManager = new HotKeyManager(m_nativeEventFilter, this);
}

void FortManager::setupTrayIcon()
{
    m_trayIcon = new TrayIcon(this);

    connect(m_trayIcon, &TrayIcon::mouseClicked, this, &FortManager::showProgramsWindow);
    connect(m_trayIcon, &TrayIcon::mouseDoubleClicked, this, &FortManager::showOptionsWindow);
    connect(m_trayIcon, &TrayIcon::mouseMiddleClicked, this, &FortManager::showStatisticsWindow);
    connect(m_trayIcon, &TrayIcon::mouseRightClicked, m_trayIcon, &TrayIcon::showTrayMenu,
            Qt::QueuedConnection);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &FortManager::onTrayMessageClicked);

    connect(confManager(), &ConfManager::confChanged, m_trayIcon, &TrayIcon::updateTrayMenu);
    connect(confManager(), &ConfManager::iniUserChanged, m_trayIcon, &TrayIcon::updateTrayMenu);
    connect(confManager(), &ConfManager::appAdded, m_trayIcon, [&](bool alerted) {
        if (alerted) {
            m_trayIcon->updateTrayIcon(true);
        }
    });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &FortManager::closeUi);
}

void FortManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow(this);
    m_progWindow->restoreWindowState();

    connect(m_progWindow, &ProgramsWindow::aboutToClose, this, &FortManager::closeProgramsWindow);
    connect(m_progWindow, &ProgramsWindow::activationChanged, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(false); });
}

void FortManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow(this);
    m_optWindow->restoreWindowState();

    connect(m_optWindow, &OptionsWindow::aboutToClose, this, &FortManager::closeOptionsWindow);
}

void FortManager::setupZonesWindow()
{
    m_zoneWindow = new ZonesWindow(this);
    m_zoneWindow->restoreWindowState();

    connect(m_zoneWindow, &ZonesWindow::aboutToClose, this, &FortManager::closeZonesWindow);
}

void FortManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow(confManager());
    m_graphWindow->restoreWindowState();

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, this,
            [&](QMouseEvent *event) { m_trayIcon->showTrayMenu(mouseEventGlobalPos(event)); });

    connect(statManager(), &StatManager::trafficAdded, m_graphWindow, &GraphWindow::addTraffic);
}

void FortManager::setupStatisticsWindow()
{
    m_statWindow = new StatisticsWindow(this);
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

    if (iniUser()->graphWindowVisible()) {
        showGraphWindow();
    }
}

void FortManager::showTrayIcon()
{
    if (!m_trayIcon) {
        setupUserSettings();
        setupTranslationManager();
        setupMainWindow();
        setupHotKeyManager();
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
    const QStringList args = settings()->appArguments();

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

    if (!settings()->isPasswordRequired())
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

    const bool checked = ok && !password.isEmpty() && confManager()->checkPassword(password);

    settings()->setPasswordChecked(checked, unlockType);

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
    QString viaVersion;
    if (!settings()->canMigrate(viaVersion)) {
        showInfoBox(tr("Please first install Fort Firewall v%1 and save Options from it.")
                            .arg(viaVersion));
        abort(); // Abort the program
    }

    confManager()->load();
}

bool FortManager::updateDriverConf(bool onlyFlags)
{
    updateLogManager(false);

    const bool res = confManager()->updateDriverConf(onlyFlags);
    if (res) {
        updateStatManager(conf());
    }

    updateLogManager(true);

    return res;
}

void FortManager::updateLogManager(bool active)
{
    logManager()->setActive(active);
}

void FortManager::updateStatManager(FirewallConf *conf)
{
    statManager()->setConf(conf);
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
