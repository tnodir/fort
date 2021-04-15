#include "fortmanager.h"

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QThreadPool>
#include <QWindow>

#include <fort_version.h>

#include "conf/confmanager.h"
#include "conf/firewallconf.h"
#include "driver/drivermanager.h"
#include "form/conn/connectionswindow.h"
#include "form/controls/mainwindow.h"
#include "form/dialog/passworddialog.h"
#include "form/graph/graphwindow.h"
#include "form/opt/optionswindow.h"
#include "form/prog/programswindow.h"
#include "form/tray/trayicon.h"
#include "form/zone/zoneswindow.h"
#include "fortsettings.h"
#include "log/logmanager.h"
#include "model/applistmodel.h"
#include "model/appstatmodel.h"
#include "model/connlistmodel.h"
#include "model/traflistmodel.h"
#include "model/zonelistmodel.h"
#include "stat/quotamanager.h"
#include "stat/statmanager.h"
#include "task/taskinfo.h"
#include "task/taskinfoupdatechecker.h"
#include "task/taskinfozonedownloader.h"
#include "task/taskmanager.h"
#include "translationmanager.h"
#include "util/app/appinfocache.h"
#include "util/app/appinfomanager.h"
#include "util/dateutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/hotkeymanager.h"
#include "util/logger.h"
#include "util/nativeeventfilter.h"
#include "util/net/hostinfocache.h"
#include "util/net/netutil.h"
#include "util/osutil.h"
#include "util/startuputil.h"
#include "util/stringutil.h"
#include "util/window/widgetwindowstatewatcher.h"

FortManager::FortManager(FortSettings *settings, EnvManager *envManager, QObject *parent) :
    QObject(parent), m_trayTriggered(false), m_settings(settings), m_envManager(envManager)
{
}

FortManager::~FortManager()
{
    closeMainWindow();

    closeDriver();
    closeLogManager();

    OsUtil::closeMutex(m_instanceMutex);
}

bool FortManager::checkRunningInstance()
{
    bool isSingleInstance;
    m_instanceMutex = OsUtil::createMutex(APP_BASE, isSingleInstance);

    if (!isSingleInstance) {
        showErrorBox(tr("Application is already running!"));
    }
    return isSingleInstance;
}

void FortManager::initialize()
{
    // Create instances
    {
        m_quotaManager = new QuotaManager(settings(), this);
        m_statManager = new StatManager(settings()->statFilePath(), m_quotaManager, this);
        m_driverManager = new DriverManager(this);
        m_confManager = new ConfManager(settings()->confFilePath(), this, this);
        m_logManager = new LogManager(this, this);
        m_taskManager = new TaskManager(this, this);
    }

    setupThreadPool();

    setupLogger();
    setupEventFilter();
    setupEnvManager();
    setupStatManager();
    setupConfManager();

    setupAppInfoCache();
    setupHostInfoCache();
    setupModels();

    setupLogManager();
    setupDriver();
    setupTaskManager();

    loadConf();
}

FirewallConf *FortManager::conf() const
{
    return confManager()->conf();
}

FirewallConf *FortManager::confToEdit() const
{
    return confManager()->confToEdit();
}

void FortManager::setupTranslationManager()
{
    TranslationManager::instance()->switchLanguageByName(settings()->language());
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(8, QThread::idealThreadCount() * 2));
}

void FortManager::installDriver()
{
    closeDriver();

    DriverManager::reinstallDriver();

    if (setupDriver()) {
        updateDriverConf();
    }
}

void FortManager::removeDriver()
{
    closeDriver();

    DriverManager::uninstallDriver();
}

bool FortManager::setupDriver()
{
    bool opened = driverManager()->openDevice();

    if (!confManager()->validateDriver()) {
        driverManager()->closeDevice();

        opened = false;
    }

    return opened;
}

void FortManager::closeDriver()
{
    updateLogManager(false);
    updateStatManager(nullptr);

    driverManager()->closeDevice();
}

void FortManager::setupAppInfoCache()
{
    QString dbPath;
    if (settings()->noCache()) {
        dbPath = ":memory:";
    } else {
        const QString cachePath = settings()->cachePath();
        dbPath = cachePath + "appinfocache.db";
    }

    AppInfoManager *manager = new AppInfoManager(this);
    manager->setupDb(dbPath);

    m_appInfoCache = new AppInfoCache(this);
    m_appInfoCache->setManager(manager);
}

void FortManager::setupHostInfoCache()
{
    m_hostInfoCache = new HostInfoCache(this);
}

void FortManager::setupModels()
{
    m_appListModel = new AppListModel(m_confManager, this);
    appListModel()->setAppInfoCache(m_appInfoCache);
    appListModel()->initialize();

    m_appStatModel = new AppStatModel(m_statManager, this);
    appStatModel()->setAppInfoCache(m_appInfoCache);
    appStatModel()->initialize();

    m_zoneListModel = new ZoneListModel(m_confManager, this);
    zoneListModel()->initialize();

    m_connListModel = new ConnListModel(m_statManager, this);
    connListModel()->setAppInfoCache(m_appInfoCache);
    connListModel()->setHostInfoCache(m_hostInfoCache);
}

void FortManager::setupLogManager()
{
    logManager()->initialize();
}

void FortManager::closeLogManager()
{
    logManager()->close();
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

void FortManager::setupStatManager()
{
    m_statManager->initialize();

    connect(m_quotaManager, &QuotaManager::alert, this, &FortManager::showInfoBox);
}

void FortManager::setupConfManager()
{
    confManager()->initialize();
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    logger->setPath(settings()->logsPath());
    logger->setActive(true);

    logger->setDebug(settings()->debug());
    logger->setConsole(settings()->console());
}

void FortManager::setupTaskManager()
{
    taskManager()->loadSettings();

    connect(taskManager()->taskInfoZoneDownloader(), &TaskInfoZoneDownloader::zonesUpdated,
            confManager(), &ConfManager::updateDriverZones);

    taskManager()->taskInfoZoneDownloader()->loadZones();
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

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &FortManager::onTrayActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &FortManager::onTrayMessageClicked);

    connect(confManager(), &ConfManager::confSaved, m_trayIcon, &TrayIcon::updateTrayMenu);
    connect(confManager(), &ConfManager::alertedAppAdded, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(true); });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &FortManager::closeUi);
}

void FortManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow(this);

    m_progWindowState = new WidgetWindowStateWatcher(this);
    m_progWindowState->install(m_progWindow);

    connect(m_progWindow, &ProgramsWindow::activationChanged, m_trayIcon,
            [&] { m_trayIcon->updateTrayIcon(false); });
}

void FortManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow(this);

    m_optWindowState = new WidgetWindowStateWatcher(this);
    m_optWindowState->install(m_optWindow);
}

void FortManager::setupZonesWindow()
{
    m_zoneWindow = new ZonesWindow(this);

    m_zoneWindowState = new WidgetWindowStateWatcher(this);
    m_zoneWindowState->install(m_zoneWindow);
}

void FortManager::setupGraphWindow()
{
    m_graphWindow = new GraphWindow(settings());

    m_graphWindowState = new WidgetWindowStateWatcher(this);
    m_graphWindowState->install(m_graphWindow);

    connect(m_graphWindow, &GraphWindow::aboutToClose, this, [&] { closeGraphWindow(); });
    connect(m_graphWindow, &GraphWindow::mouseRightClick, m_trayIcon, &TrayIcon::showTrayMenu);

    connect(m_statManager, &StatManager::trafficAdded, m_graphWindow, &GraphWindow::addTraffic);
}

void FortManager::setupConnectionsWindow()
{
    m_connWindow = new ConnectionsWindow(this);

    m_connWindowState = new WidgetWindowStateWatcher(this);
    m_connWindowState->install(m_connWindow);
}

void FortManager::closeUi()
{
    closeGraphWindow(true);
    closeOptionsWindow();
    closeProgramsWindow();
    closeZonesWindow();
    closeConnectionsWindow();
    closeTrayIcon();
}

void FortManager::show()
{
    showTrayIcon();

    if (settings()->graphWindowVisible()) {
        showGraphWindow();
    }
}

void FortManager::showTrayIcon()
{
    if (!m_trayIcon) {
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
        restoreProgWindowState();
    }

    m_progWindow->show();
    m_progWindow->raise();
    m_progWindow->activateWindow();
}

void FortManager::closeProgramsWindow()
{
    if (!m_progWindow)
        return;

    saveProgWindowState();

    m_progWindow->hide();

    m_progWindow->deleteLater();
    m_progWindow = nullptr;
}

bool FortManager::showProgramEditForm(const QString &appPath)
{
    showProgramsWindow();
    if (!(m_progWindow && m_progWindow->isVisible()))
        return false; // May be not opened due to password checking

    if (!m_progWindow->openAppEditFormByPath(appPath)) {
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
        confManager()->initConfToEdit();

        setupOptionsWindow();
        restoreOptWindowState();

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

    saveOptWindowState();

    m_optWindow->hide();

    m_optWindow->deleteLater();
    m_optWindow = nullptr;

    confManager()->setConfToEdit(nullptr);

    emit optWindowChanged(false);
}

void FortManager::showZonesWindow()
{
    if (!(m_zoneWindow && m_zoneWindow->isVisible()) && !checkPassword())
        return;

    if (!m_zoneWindow) {
        setupZonesWindow();
        restoreZoneWindowState();
    }

    m_zoneWindow->show();
    m_zoneWindow->raise();
    m_zoneWindow->activateWindow();
}

void FortManager::closeZonesWindow()
{
    if (!m_zoneWindow)
        return;

    saveZoneWindowState();

    m_zoneWindow->hide();

    m_zoneWindow->deleteLater();
    m_zoneWindow = nullptr;
}

void FortManager::showGraphWindow()
{
    if (!m_graphWindow) {
        setupGraphWindow();
    }

    m_graphWindow->show();

    emit graphWindowChanged(true);

    restoreGraphWindowState();
}

void FortManager::closeGraphWindow(bool storeVisibility)
{
    if (!m_graphWindow)
        return;

    saveGraphWindowState(storeVisibility);

    m_graphWindowState->uninstall(m_graphWindow);

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

void FortManager::updateGraphWindow()
{
    if (!m_graphWindow)
        return;

    m_graphWindow->updateColors();
    m_graphWindow->updateWindowFlags();
}

void FortManager::showConnectionsWindow()
{
    if (!(m_connWindow && m_connWindow->isVisible()) && !checkPassword())
        return;

    if (!m_connWindow) {
        setupConnectionsWindow();
        restoreConnWindowState();
    }

    m_connWindow->show();
    m_connWindow->raise();
    m_connWindow->activateWindow();
}

void FortManager::closeConnectionsWindow()
{
    if (!m_connWindow)
        return;

    saveConnWindowState();

    m_connWindow->hide();

    m_connWindow->deleteLater();
    m_connWindow = nullptr;
}

void FortManager::processRestartRequired()
{
    if (!showYesNoBox(tr("Restart Required"), tr("Restart Now"), tr("Later")))
        return;

    const QString appFilePath = QCoreApplication::applicationFilePath();
    const QStringList args = settings()->appArguments();

    connect(this, &QObject::destroyed, [=] {
        StartupUtil::startService(); // Try to start the (maybe installed) service
        QProcess::startDetached(appFilePath, args);
    });

    QCoreApplication::quit();
}

void FortManager::quitByCheckPassword()
{
    if (!checkPassword())
        return;

    closeUi();

    QCoreApplication::quit();
}

bool FortManager::checkPassword()
{
    static bool g_passwordDialogOpened = false;

    if (!settings()->isPasswordRequired())
        return true;

    if (g_passwordDialogOpened) {
        auto dialog = qApp->activeModalWidget();
        if (dialog) {
            dialog->activateWindow();
        }
        return false;
    }

    g_passwordDialogOpened = true;

    QString password;
    PasswordDialog::UnlockType unlockType = PasswordDialog::UnlockDisabled;
    const bool ok = PasswordDialog::getPassword(password, unlockType, m_mainWindow);

    g_passwordDialogOpened = false;

    const bool checked = ok && !password.isEmpty()
            && StringUtil::cryptoHash(password) == settings()->passwordHash();

    settings()->setPasswordChecked(checked, checked ? unlockType : PasswordDialog::UnlockDisabled);

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

bool FortManager::saveOriginConf(const QString &message, bool onlyFlags)
{
    if (!saveConf(conf(), onlyFlags))
        return false;

    closeOptionsWindow();

    if (!message.isEmpty()) {
        showTrayMessage(message);
    }

    return true;
}

bool FortManager::saveConf(bool onlyFlags)
{
    Q_ASSERT(confToEdit());

    return saveConf(confToEdit(), onlyFlags);
}

bool FortManager::applyConf(bool onlyFlags)
{
    if (!saveConf(onlyFlags))
        return false;

    Q_ASSERT(!confToEdit());

    confManager()->initConfToEdit();

    return true;
}

bool FortManager::applyConfImmediateFlags()
{
    if (confToEdit()) {
        conf()->copyImmediateFlags(*confToEdit());
    }

    return saveConf(conf(), true);
}

bool FortManager::loadConf()
{
    QString viaVersion;
    if (!settings()->confCanMigrate(viaVersion)) {
        showInfoBox(tr("Please first install Fort Firewall v%1 and save Options from it.")
                            .arg(viaVersion));
        abort(); //  Abort the program
    }

    if (!confManager()->load(*conf()))
        return false;

    return updateDriverConf();
}

bool FortManager::saveConf(FirewallConf *newConf, bool onlyFlags)
{
    if (!confManager()->save(*newConf, onlyFlags))
        return false;

    updateDriverConf(onlyFlags);

    return true;
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
    m_statManager->setFirewallConf(conf);
}

void FortManager::saveProgWindowState()
{
    settings()->setProgWindowGeometry(m_progWindowState->geometry());
    settings()->setProgWindowMaximized(m_progWindowState->maximized());

    emit afterSaveProgWindowState();
}

void FortManager::restoreProgWindowState()
{
    m_progWindowState->restore(m_progWindow, QSize(1024, 768), settings()->progWindowGeometry(),
            settings()->progWindowMaximized());

    emit afterRestoreProgWindowState();
}

void FortManager::saveOptWindowState()
{
    settings()->setOptWindowGeometry(m_optWindowState->geometry());
    settings()->setOptWindowMaximized(m_optWindowState->maximized());

    emit afterSaveOptWindowState();
}

void FortManager::restoreOptWindowState()
{
    m_optWindowState->restore(m_optWindow, QSize(1024, 768), settings()->optWindowGeometry(),
            settings()->optWindowMaximized());

    emit afterRestoreOptWindowState();
}

void FortManager::saveZoneWindowState()
{
    settings()->setZoneWindowGeometry(m_zoneWindowState->geometry());
    settings()->setZoneWindowMaximized(m_zoneWindowState->maximized());

    emit afterSaveZoneWindowState();
}

void FortManager::restoreZoneWindowState()
{
    m_zoneWindowState->restore(m_zoneWindow, QSize(1024, 768), settings()->zoneWindowGeometry(),
            settings()->zoneWindowMaximized());

    emit afterRestoreZoneWindowState();
}

void FortManager::saveGraphWindowState(bool visible)
{
    settings()->setGraphWindowVisible(visible);
    settings()->setGraphWindowGeometry(m_graphWindowState->geometry());
    settings()->setGraphWindowMaximized(m_graphWindowState->maximized());
}

void FortManager::restoreGraphWindowState()
{
    m_graphWindowState->restore(m_graphWindow, QSize(400, 300), settings()->graphWindowGeometry(),
            settings()->graphWindowMaximized());
}

void FortManager::saveConnWindowState()
{
    settings()->setConnWindowGeometry(m_connWindowState->geometry());
    settings()->setConnWindowMaximized(m_connWindowState->maximized());

    emit afterSaveConnWindowState();
}

void FortManager::restoreConnWindowState()
{
    m_connWindowState->restore(m_connWindow, QSize(1024, 768), settings()->connWindowGeometry(),
            settings()->connWindowMaximized());

    emit afterRestoreConnWindowState();
}

void FortManager::onTrayActivated(int reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        m_trayTriggered = false;
        QTimer::singleShot(QApplication::doubleClickInterval(), this, [this] {
            if (!m_trayTriggered) {
                m_trayTriggered = true;
                showProgramsWindow();
            }
        });
        break;
    case QSystemTrayIcon::DoubleClick:
        if (!m_trayTriggered) {
            m_trayTriggered = true;
            showOptionsWindow();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        showConnectionsWindow();
        break;
    default:
        break;
    }
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
