#include "fortmanager.h"

#include <QApplication>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTimer>
#include <QWindow>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/confmanager.h"
#include "conf/firewallconf.h"
#include "driver/drivermanager.h"
#include "form/graph/graphwindow.h"
#include "form/opt/optionswindow.h"
#include "form/prog/programswindow.h"
#include "form/zone/zoneswindow.h"
#include "fortsettings.h"
#include "log/logmanager.h"
#include "model/applistmodel.h"
#include "model/appstatmodel.h"
#include "model/iplistmodel.h"
#include "model/traflistmodel.h"
#include "model/zonelistmodel.h"
#include "stat/quotamanager.h"
#include "stat/statmanager.h"
#include "task/taskinfo.h"
#include "task/taskinfoupdatechecker.h"
#include "task/taskmanager.h"
#include "translationmanager.h"
#include "util/app/appinfocache.h"
#include "util/app/appinfomanager.h"
#include "util/dateutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/guiutil.h"
#include "util/hotkeymanager.h"
#include "util/logger.h"
#include "util/nativeeventfilter.h"
#include "util/net/hostinfocache.h"
#include "util/net/netutil.h"
#include "util/osutil.h"
#include "util/stringutil.h"
#include "util/window/widgetwindowstatewatcher.h"

FortManager::FortManager(FortSettings *fortSettings,
                         QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_progWindowState(new WidgetWindowStateWatcher(this)),
    m_optWindowState(new WidgetWindowStateWatcher(this)),
    m_zoneWindowState(new WidgetWindowStateWatcher(this)),
    m_graphWindowState(new WidgetWindowStateWatcher(this)),
    m_settings(fortSettings),
    m_quotaManager(new QuotaManager(fortSettings, this)),
    m_statManager(new StatManager(fortSettings->statFilePath(),
                                  m_quotaManager, this)),
    m_driverManager(new DriverManager(this)),
    m_envManager(new EnvManager(this)),
    m_confManager(new ConfManager(fortSettings->confFilePath(), this, this)),
    m_logManager(new LogManager(this, this)),
    m_nativeEventFilter(new NativeEventFilter(this)),
    m_hotKeyManager(new HotKeyManager(m_nativeEventFilter, this)),
    m_taskManager(new TaskManager(this, this)),
    m_appInfoCache(new AppInfoCache(this)),
    m_appListModel(new AppListModel(m_confManager, this)),
    m_appStatModel(new AppStatModel(m_statManager, this)),
    m_zoneListModel(new ZoneListModel(m_confManager, this))
{
    setupTranslationManager();
    setupThreadPool();

    setupLogger();
    setupAppInfoCache();
    setupEnvManager();
    setupStatManager();
    setupConfManager();

    setupModels();

    setupLogManager();
    setupDriver();

    loadConf();

    setupTaskManager();
    setupTrayIcon();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &FortManager::closeUi);
}

FortManager::~FortManager()
{
    removeHotKeys();

    closeDriver();
    closeLogManager();
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
    TranslationManager::instance()->switchLanguageByName(
                settings()->language());

    connect(TranslationManager::instance(), &TranslationManager::languageChanged,
            this, &FortManager::retranslateTrayMenu);
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(
                qMax(8, QThread::idealThreadCount() * 2));
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

void FortManager::setupModels()
{
    appListModel()->setAppInfoCache(m_appInfoCache);
    appListModel()->initialize();

    appStatModel()->setAppInfoCache(m_appInfoCache);
    appStatModel()->initialize();

    zoneListModel()->initialize();
}

void FortManager::setupLogManager()
{
    logManager()->initialize();
}

void FortManager::closeLogManager()
{
    logManager()->close();
}

void FortManager::setupEnvManager()
{
    connect(m_nativeEventFilter, &NativeEventFilter::environmentChanged,
            envManager(), &EnvManager::onEnvironmentChanged);

    connect(envManager(), &EnvManager::environmentUpdated, this, [&] {
        updateDriverConf();
    });
}

void FortManager::setupStatManager()
{
    m_statManager->initialize();

    connect(m_quotaManager, &QuotaManager::alert,
            this, &FortManager::showInfoBox);
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
}

void FortManager::setupTrayIcon()
{
    m_trayIcon->setToolTip(QGuiApplication::applicationDisplayName());
    m_trayIcon->setIcon(QIcon(":/images/sheild-96.png"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
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
        case QSystemTrayIcon::Unknown:
        case QSystemTrayIcon::Context:
        case QSystemTrayIcon::MiddleClick:
            break;
        }
    });

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
            this, &FortManager::showProgramsWindow);

    connect(confManager(), &ConfManager::confSaved,
            this, &FortManager::updateTrayMenu);
    connect(this, &FortManager::optWindowChanged,
            this, &FortManager::updateTrayMenuFlags);

    updateTrayMenu();
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

    m_appInfoCache->setManager(manager);
}

bool FortManager::setupProgramsWindow()
{
    m_progWindow = new ProgramsWindow(this);

    m_progWindowState->install(m_progWindow);

    return true;
}

bool FortManager::setupOptionsWindow()
{
    m_optWindow = new OptionsWindow(this);

    m_optWindowState->install(m_optWindow);

    return true;
}

bool FortManager::setupZonesWindow()
{
    m_zoneWindow = new ZonesWindow(this);

    m_zoneWindowState->install(m_zoneWindow);

    return true;
}

void FortManager::closeUi()
{
    closeGraphWindow(true);
    closeOptionsWindow();
    closeProgramsWindow();
    closeZonesWindow();
}

void FortManager::launch()
{
    showTrayIcon();

    if (settings()->graphWindowVisible()) {
        showGraphWindow();
    }
}

void FortManager::showTrayIcon()
{
    m_trayIcon->show();
}

void FortManager::showTrayMessage(const QString &message)
{
    m_trayIcon->showMessage(QGuiApplication::applicationDisplayName(), message);
}

void FortManager::showTrayMenu(QMouseEvent *event)
{
    QMenu *menu = m_trayIcon->contextMenu();
    if (!menu) return;

    menu->popup(event->globalPos());
}

void FortManager::showProgramsWindow()
{
    if (!(m_progWindow && m_progWindow->isVisible())
            && !checkPassword())
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

void FortManager::showOptionsWindow()
{
    if (!(m_optWindow && m_optWindow->isVisible())
            && !checkPassword())
        return;

    if (!m_optWindow) {
        confManager()->initConfToEdit();

        setupOptionsWindow();
        restoreOptWindowState();

        emit optWindowChanged();
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

    emit optWindowChanged();
}

void FortManager::showZonesWindow()
{
    if (!(m_zoneWindow && m_zoneWindow->isVisible())
            && !checkPassword())
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
        m_graphWindow = new GraphWindow(settings());

        m_graphWindowState->install(m_graphWindow);

        connect(m_graphWindow, &GraphWindow::aboutToClose, [this] {
            closeGraphWindow();
        });

        connect(m_graphWindow, &GraphWindow::mouseRightClick,
                this, &FortManager::showTrayMenu);

        connect(m_statManager, &StatManager::trafficAdded,
                m_graphWindow, &GraphWindow::addTraffic);
    }

    m_graphWindow->show();

    m_graphWindowAction->setChecked(true);

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

    m_graphWindowAction->setChecked(false);
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

void FortManager::exit(int retcode)
{
    if (!checkPassword())
        return;

    closeUi();

    QCoreApplication::exit(retcode);
}

bool FortManager::checkPassword()
{
    static bool g_passwordDialogOpened = false;

    const QString passwordHash = settings()->passwordHash();
    if (passwordHash.isEmpty())
        return true;

    if (g_passwordDialogOpened) {
        auto dialog = qApp->activeModalWidget();
        if (dialog != nullptr) {
            dialog->activateWindow();
        }
        return false;
    }

    g_passwordDialogOpened = true;

    const QString password = QInputDialog::getText(
                &m_window, tr("Password input"), tr("Please enter the password"),
                QLineEdit::Password);

    g_passwordDialogOpened = false;

    return !password.isEmpty()
            && StringUtil::cryptoHash(password) == passwordHash;
}

void FortManager::showErrorBox(const QString &text,
                               const QString &title)
{
    QMessageBox::warning(&m_window, title, text);
}

void FortManager::showInfoBox(const QString &text,
                              const QString &title)
{
    QMessageBox::information(&m_window, title, text);
}

bool FortManager::showQuestionBox(const QString &text,
                                  const QString &title)
{
    return QMessageBox::question(&m_window, title, text) == QMessageBox::Yes;
}

bool FortManager::saveOriginConf(const QString &message)
{
    if (!saveConf(conf()))
        return false;

    closeOptionsWindow();
    showTrayMessage(message);
    return true;
}

bool FortManager::saveConf(bool onlyFlags)
{
    Q_ASSERT(confToEdit() != nullptr);

    return saveConf(confToEdit(), onlyFlags);
}

bool FortManager::applyConf(bool onlyFlags)
{
    if (!saveConf(onlyFlags))
        return false;

    Q_ASSERT(confToEdit() == nullptr);

    confManager()->initConfToEdit();

    return true;
}

bool FortManager::applyConfImmediateFlags()
{
    if (confToEdit() != nullptr) {
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
        abort();  //  Abort the program
    }

    if (!confManager()->load(*conf()))
        return false;

    return updateDriverConf();
}

bool FortManager::saveConf(FirewallConf *newConf, bool onlyFlags)
{
    if (!confManager()->save(*newConf, onlyFlags))
        return false;

    return updateDriverConf(onlyFlags);
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

void FortManager::saveTrayFlags()
{
    conf()->setFilterEnabled(m_filterEnabledAction->isChecked());
    conf()->setStopTraffic(m_stopTrafficAction->isChecked());
    conf()->setStopInetTraffic(m_stopInetTrafficAction->isChecked());
    conf()->setAllowAllNew(m_allowAllNewAction->isChecked());

    int i = 0;
    for (AppGroup *appGroup : conf()->appGroups()) {
        const QAction *action = m_appGroupActions.at(i);
        appGroup->setEnabled(action->isChecked());
        ++i;
    }

    saveConf(conf(), true);
}

void FortManager::saveProgWindowState()
{
    settings()->setProgWindowGeometry(m_progWindowState->geometry());
    settings()->setProgWindowMaximized(m_progWindowState->maximized());

    emit afterSaveProgWindowState();
}

void FortManager::restoreProgWindowState()
{
    m_progWindowState->restore(m_progWindow, QSize(1024, 768),
                               settings()->progWindowGeometry(),
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
    m_optWindowState->restore(m_optWindow, QSize(1024, 768),
                              settings()->optWindowGeometry(),
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
    m_zoneWindowState->restore(m_zoneWindow, QSize(1024, 768),
                               settings()->zoneWindowGeometry(),
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
    m_graphWindowState->restore(m_graphWindow, QSize(400, 300),
                                settings()->graphWindowGeometry(),
                                settings()->graphWindowMaximized());
}

void FortManager::updateTrayMenu(bool onlyFlags)
{
    QMenu *oldMenu = m_trayIcon->contextMenu();
    if (oldMenu != nullptr && !onlyFlags) {
        oldMenu->deleteLater();
        oldMenu = nullptr;

        removeHotKeys();
    }

    if (oldMenu == nullptr) {
        createTrayMenu();
        retranslateTrayMenu();
    }

    updateTrayMenuFlags();
}

void FortManager::createTrayMenu()
{
    const bool hotKeyEnabled = settings()->hotKeyEnabled();

    QMenu *menu = new QMenu(&m_window);

    m_programsAction = addAction(
                menu, QIcon(":/images/application_cascade.png"), QString(),
                this, SLOT(showProgramsWindow()));
    addHotKey(m_programsAction, settings()->hotKeyPrograms(), hotKeyEnabled);

    m_optionsAction = addAction(
                menu, QIcon(":/images/cog.png"), QString(),
                this, SLOT(showOptionsWindow()));
    addHotKey(m_optionsAction, settings()->hotKeyOptions(), hotKeyEnabled);

    m_zonesAction = addAction(
                menu, QIcon(":/images/map.png"), QString(),
                this, SLOT(showZonesWindow()));
    addHotKey(m_zonesAction, settings()->hotKeyZones(), hotKeyEnabled);

    m_graphWindowAction = addAction(
                menu, QIcon(":/images/chart_bar.png"), QString(),
                this, SLOT(switchGraphWindow()), true,
                (m_graphWindow != nullptr));
    addHotKey(m_graphWindowAction, settings()->hotKeyGraph(),
              conf()->logStat());

    menu->addSeparator();

    m_filterEnabledAction = addAction(
                menu, QIcon(), QString(),
                this, SLOT(saveTrayFlags()), true);
    addHotKey(m_filterEnabledAction,
              settings()->hotKeyFilter(), hotKeyEnabled);

    m_stopTrafficAction = addAction(
                menu, QIcon(), QString(),
                this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopTrafficAction,
              settings()->hotKeyStopTraffic(), hotKeyEnabled);

    m_stopInetTrafficAction = addAction(
                menu, QIcon(), QString(),
                this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopInetTrafficAction,
              settings()->hotKeyStopInetTraffic(), hotKeyEnabled);

    m_allowAllNewAction = addAction(
                menu, QIcon(), QString(),
                this, SLOT(saveTrayFlags()), true);
    addHotKey(m_allowAllNewAction,
              settings()->hotKeyAllowAllNew(), hotKeyEnabled);

    menu->addSeparator();

    m_appGroupActions.clear();
    int appGroupIndex = 0;
    for (const AppGroup *appGroup : conf()->appGroups()) {
        QAction *a = addAction(
                    menu, QIcon(":/images/application_double.png"),
                    appGroup->menuLabel(), this, SLOT(saveTrayFlags()), true);

        const QString shortcutText =
                settings()->hotKeyAppGroupModifiers()
                + "+F" + QString::number(++appGroupIndex);

        addHotKey(a, shortcutText, hotKeyEnabled);

        m_appGroupActions.append(a);
    }

    menu->addSeparator();
    m_quitAction = addAction(
                menu, QIcon(":/images/cross.png"), tr("Quit"),
                this, SLOT(exit()));
    addHotKey(m_quitAction, settings()->hotKeyQuit(), hotKeyEnabled);

    m_trayIcon->setContextMenu(menu);
}

void FortManager::updateTrayMenuFlags()
{
    const bool hasPassword = settings()->hasPassword();
    const bool editEnabled = (!hasPassword && m_optWindow == nullptr);

    m_filterEnabledAction->setEnabled(editEnabled);
    m_stopTrafficAction->setEnabled(editEnabled);
    m_stopInetTrafficAction->setEnabled(editEnabled);
    m_allowAllNewAction->setEnabled(editEnabled);

    m_filterEnabledAction->setChecked(conf()->filterEnabled());
    m_stopTrafficAction->setChecked(conf()->stopTraffic());
    m_stopInetTrafficAction->setChecked(conf()->stopInetTraffic());
    m_allowAllNewAction->setChecked(conf()->allowAllNew());

    int appGroupIndex = 0;
    for (QAction *action : m_appGroupActions) {
        const auto appGroup = conf()->appGroups().at(appGroupIndex++);

        action->setEnabled(editEnabled);
        action->setChecked(appGroup->enabled());
    }
}

void FortManager::retranslateTrayMenu()
{
    m_programsAction->setText(tr("Programs"));
    m_optionsAction->setText(tr("Options"));
    m_zonesAction->setText(tr("Zones"));
    m_graphWindowAction->setText(tr("Traffic Graph"));

    m_filterEnabledAction->setText(tr("Filter Enabled"));
    m_stopTrafficAction->setText(tr("Stop Traffic"));
    m_stopInetTrafficAction->setText(tr("Stop Internet Traffic"));
    m_allowAllNewAction->setText(tr("Auto-Allow All New Programs"));

    m_quitAction->setText(tr("Quit"));
}

void FortManager::addHotKey(QAction *action, const QString &shortcutText,
                            bool hotKeyEnabled)
{
    if (hotKeyEnabled && !shortcutText.isEmpty()) {
        const QKeySequence shortcut = QKeySequence::fromString(shortcutText);

        m_hotKeyManager->addAction(action, shortcut);
    }
}

void FortManager::removeHotKeys()
{
    m_hotKeyManager->removeActions();
}

QAction *FortManager::addAction(QWidget *widget,
                                const QIcon &icon, const QString &text,
                                const QObject *receiver, const char *member,
                                bool checkable, bool checked)
{
    auto action = new QAction(icon, text, widget);

    if (receiver) {
        connect(action, SIGNAL(triggered(bool)), receiver, member);
    }
    if (checkable) {
        setActionCheckable(action, checked);
    }

    widget->addAction(action);

    return action;
}

void FortManager::setActionCheckable(QAction *action, bool checked,
                                     const QObject *receiver, const char *member)
{
    action->setCheckable(true);
    action->setChecked(checked);

    if (receiver) {
        connect(action, SIGNAL(toggled(bool)), receiver, member);
    }
}
