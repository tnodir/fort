#include "fortmanager.h"

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTimer>
#include <QWindow>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "driver/drivermanager.h"
#include "fortsettings.h"
#include "graph/graphwindow.h"
#include "log/logmanager.h"
#include "log/model/appblockedmodel.h"
#include "log/model/appstatmodel.h"
#include "log/model/iplistmodel.h"
#include "log/model/traflistmodel.h"
#include "stat/quotamanager.h"
#include "stat/statmanager.h"
#include "task/taskinfo.h"
#include "task/taskmanager.h"
#include "translationmanager.h"
#include "util/app/appiconprovider.h"
#include "util/app/appinfocache.h"
#include "util/app/appinfomanager.h"
#include "util/dateutil.h"
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
#include "util/window/windowstatewatcher.h"

FortManager::FortManager(FortSettings *fortSettings,
                         QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_engine(nullptr),
    m_appWindow(nullptr),
    m_appWindowState(new WindowStateWatcher(this)),
    m_graphWindow(nullptr),
    m_graphWindowState(new WidgetWindowStateWatcher(this)),
    m_fortSettings(fortSettings),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullConf()),
    m_graphWindowAction(nullptr),
    m_filterEnabledAction(nullptr),
    m_stopTrafficAction(nullptr),
    m_stopInetTrafficAction(nullptr),
    m_quotaManager(new QuotaManager(fortSettings, this)),
    m_statManager(new StatManager(fortSettings->statFilePath(),
                                  m_quotaManager, this)),
    m_driverManager(new DriverManager(this)),
    m_logManager(new LogManager(m_statManager,
                                m_driverManager->driverWorker(), this)),
    m_nativeEventFilter(new NativeEventFilter(this)),
    m_hotKeyManager(new HotKeyManager(m_nativeEventFilter, this)),
    m_taskManager(new TaskManager(this, this)),
    m_appInfoCache(new AppInfoCache(this))
{
    setupThreadPool();

    setupLogger();
    setupAppInfoCache();
    setupStatManager();

    setupLogManager();
    setupDriver();

    loadSettings(m_firewallConf);

    m_taskManager->loadSettings(m_fortSettings);

    registerQmlTypes();

    setupTranslationManager();
    setupTrayIcon();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &FortManager::closeUi);
}

FortManager::~FortManager()
{
    removeHotKeys();

    closeDriver();
    closeLogManager();
}

void FortManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<DriverManager>("com.fortfirewall", 1, 0, "DriverManager",
                                              "Singleton");
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterUncreatableType<LogManager>("com.fortfirewall", 1, 0, "LogManager",
                                           "Singleton");
    qmlRegisterUncreatableType<AppBlockedModel>("com.fortfirewall", 1, 0, "AppBlockedModel",
                                                "Singleton");
    qmlRegisterUncreatableType<AppStatModel>("com.fortfirewall", 1, 0, "AppStatModel",
                                                "Singleton");
    qmlRegisterUncreatableType<IpListModel>("com.fortfirewall", 1, 0, "IpListModel",
                                            "Singleton");
    qmlRegisterUncreatableType<TrafListModel>("com.fortfirewall", 1, 0, "TrafListModel",
                                              "Singleton");

    qmlRegisterUncreatableType<TranslationManager>("com.fortfirewall", 1, 0, "TranslationManager",
                                                   "Singleton");
    qmlRegisterUncreatableType<TaskManager>("com.fortfirewall", 1, 0, "TaskManager",
                                            "Singleton");
    qmlRegisterUncreatableType<TaskInfo>("com.fortfirewall", 1, 0, "TaskInfo",
                                         "Singleton");

    qmlRegisterType<AddressGroup>("com.fortfirewall", 1, 0, "AddressGroup");
    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");

    qRegisterMetaType<AppInfo>();
    qmlRegisterType<AppInfoCache>("com.fortfirewall", 1, 0, "AppInfoCache");
    qmlRegisterType<DateUtil>("com.fortfirewall", 1, 0, "DateUtil");
    qmlRegisterType<FileUtil>("com.fortfirewall", 1, 0, "FileUtil");
    qmlRegisterType<GuiUtil>("com.fortfirewall", 1, 0, "GuiUtil");
    qmlRegisterType<HostInfoCache>("com.fortfirewall", 1, 0, "HostInfoCache");
    qmlRegisterType<NetUtil>("com.fortfirewall", 1, 0, "NetUtil");
    qmlRegisterType<OsUtil>("com.fortfirewall", 1, 0, "OsUtil");
    qmlRegisterType<StringUtil>("com.fortfirewall", 1, 0, "StringUtil");
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
        updateDriverConf(m_firewallConf);
    }
}

void FortManager::removeDriver()
{
    closeDriver();

    DriverManager::uninstallDriver();
}

bool FortManager::setupDriver()
{
    bool opened = m_driverManager->openDevice();

    if (!m_driverManager->validate()) {
        m_driverManager->closeDevice();

        opened = false;
    }

    return opened;
}

void FortManager::closeDriver()
{
    updateLogManager(false);
    updateStatManager(nullptr);

    m_driverManager->closeDevice();
}

void FortManager::setupLogManager()
{
    m_logManager->initialize();
}

void FortManager::closeLogManager()
{
    m_logManager->close();
}

void FortManager::setupStatManager()
{
    m_statManager->initialize();

    connect(m_quotaManager, &QuotaManager::alert,
            this, &FortManager::showInfoBox);
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    logger->setPath(m_fortSettings->logsPath());
    logger->setActive(true);

    updateLogger();
}

void FortManager::setupTranslationManager()
{
    TranslationManager::instance()->switchLanguageByName(
                m_fortSettings->language());
}

void FortManager::setupTrayIcon()
{
    m_trayIcon->setToolTip(QGuiApplication::applicationDisplayName());
    m_trayIcon->setIcon(QIcon(":/images/sheild-96.png"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showWindow();
        }
    });

    updateTrayMenu();
}

void FortManager::setupAppInfoCache()
{
    QString dbPath;
    if (m_fortSettings->isPortable()) {
        dbPath = ":memory:";
    } else {
        const QString cachePath = FileUtil::appCacheLocation();
        FileUtil::makePath(cachePath);
        dbPath = cachePath + "/appinfocache.db";
    }

    AppInfoManager *manager = new AppInfoManager(this);
    manager->setupDb(dbPath);

    m_appInfoCache->setManager(manager);
}

bool FortManager::setupEngine()
{
    if (m_fortSettings->isPortable()) {
        qputenv("QML_DISABLE_DISK_CACHE", "1");
        qputenv("QT_DISABLE_SHADER_DISK_CACHE", "1");
    }

    m_engine = new QQmlApplicationEngine(this);

    QQmlContext *context = m_engine->rootContext();
    context->setContextProperty("fortManager", this);
    context->setContextProperty("driverManager", m_driverManager);
    context->setContextProperty("translationManager", TranslationManager::instance());
    context->setContextProperty("appInfoCache", m_appInfoCache);

    m_engine->addImageProvider(AppIconProvider::id(),
                               new AppIconProvider(m_appInfoCache->manager()));

    m_engine->load(QUrl("qrc:/qml/main.qml"));

    const QList<QObject *> rootObjects = m_engine->rootObjects();

    if (rootObjects.isEmpty()) {
        showErrorBox("Cannot setup QML Engine");
        return false;
    }

    m_appWindow = qobject_cast<QWindow *>(rootObjects.first());
    Q_ASSERT(m_appWindow);

    m_appWindowState->install(m_appWindow);

    return true;
}

void FortManager::closeEngine()
{
    m_appWindow = nullptr;

    if (m_engine) {
        m_engine->deleteLater();
        m_engine = nullptr;
    }
}

void FortManager::closeUi()
{
    closeGraphWindow(true);
    closeWindow();

    closeEngine();
}

void FortManager::launch()
{
    showTrayIcon();

    if (m_fortSettings->graphWindowVisible()) {
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

void FortManager::showWindow()
{
    if (!m_engine) {
        setupEngine();
    }

    if (!m_appWindow || !(m_appWindow->isVisible()
                          || checkPassword()))
        return;

    if (m_firewallConfToEdit == nullConf()) {
        setFirewallConfToEdit(cloneConf(*m_firewallConf));
    }

    m_appWindow->show();
    m_appWindow->raise();
    m_appWindow->requestActivate();

    restoreWindowState();
}

void FortManager::closeWindow()
{
    if (!m_appWindow)
        return;

    saveWindowState();

    m_appWindow->hide();

    setFirewallConfToEdit(nullConf());
}

void FortManager::showGraphWindow()
{
    if (!m_graphWindow) {
        m_graphWindow = new GraphWindow(m_fortSettings);

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
    closeUi();

    QCoreApplication::exit(retcode);
}

bool FortManager::checkPassword()
{
    const QString passwordHash = fortSettings()->passwordHash();
    if (passwordHash.isEmpty())
        return true;

    const QString password = QInputDialog::getText(
                &m_window, tr("Password input"), tr("Please enter the password"),
                QLineEdit::Password);

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

QStringList FortManager::getOpenFileNames(const QString &title,
                                          const QString &filter)
{
    return QFileDialog::getOpenFileNames(
                nullptr, title, QString(), filter,
                nullptr, QFileDialog::ReadOnly);
}

bool FortManager::saveOriginConf(const QString &message)
{
    if (!saveSettings(m_firewallConf))
        return false;

    closeWindow();
    showTrayMessage(message);
    return true;
}

bool FortManager::saveConf(bool onlyFlags)
{
    return saveSettings(m_firewallConfToEdit, onlyFlags);
}

bool FortManager::applyConf(bool onlyFlags)
{
    Q_ASSERT(m_firewallConfToEdit != nullConf());

    FirewallConf *newConf = cloneConf(*m_firewallConfToEdit);

    return saveSettings(newConf, onlyFlags);
}

bool FortManager::applyConfImmediateFlags()
{
    Q_ASSERT(m_firewallConfToEdit != nullConf());

    m_firewallConf->copyImmediateFlags(*m_firewallConfToEdit);

    return saveSettings(m_firewallConf, true, true);
}

void FortManager::setFirewallConfToEdit(FirewallConf *conf)
{
    if (m_firewallConfToEdit == conf)
        return;

    if (m_firewallConfToEdit != nullConf()
            && m_firewallConfToEdit != m_firewallConf) {
        m_firewallConfToEdit->deleteLater();
    }

    m_firewallConfToEdit = conf;
    emit firewallConfToEditChanged();

    updateTrayMenu();
}

bool FortManager::loadSettings(FirewallConf *conf)
{
    bool isNewConf;

    if (!m_fortSettings->readConf(*conf, isNewConf)) {
        showErrorBox("Load Settings: " + m_fortSettings->errorMessage());
        return false;
    }

    if (isNewConf) {
        conf->setupDefault();
    }

    return updateDriverConf(conf);
}

bool FortManager::saveSettings(FirewallConf *newConf, bool onlyFlags,
                               bool immediateFlags)
{
    if (!(onlyFlags ? m_fortSettings->writeConfIni(*newConf)
          : m_fortSettings->writeConf(*newConf))) {
        showErrorBox("Save Settings: " + m_fortSettings->errorMessage());
        return false;
    }

    if (m_firewallConf != newConf) {
        m_firewallConf->deleteLater();
        m_firewallConf = newConf;
    }

    if (!immediateFlags) {
        updateLogger();
        updateTrayMenu();
    }

    return updateDriverConf(m_firewallConf, onlyFlags);
}

bool FortManager::updateDriverConf(FirewallConf *conf, bool onlyFlags)
{
    if (!m_driverManager->isDeviceOpened())
        return true;

    updateLogManager(false);

    // Update driver
    const bool res = onlyFlags
            ? m_driverManager->writeConfFlags(*conf)
            : m_driverManager->writeConf(*conf);

    if (res) {
        updateStatManager(conf);
        updateLogManager(true);
    } else {
        closeDriver();
    }

    return res;
}

void FortManager::updateLogManager(bool active)
{
    m_logManager->setActive(active);
}

void FortManager::updateStatManager(FirewallConf *conf)
{
    m_statManager->setFirewallConf(conf);
}

void FortManager::setLanguage(int language)
{
    if (!TranslationManager::instance()->switchLanguage(language))
        return;

    m_fortSettings->setLanguage(TranslationManager::instance()->localeName());

    updateTrayMenu();
}

void FortManager::saveTrayFlags()
{
    m_firewallConf->setFilterEnabled(m_filterEnabledAction->isChecked());
    m_firewallConf->setStopTraffic(m_stopTrafficAction->isChecked());
    m_firewallConf->setStopInetTraffic(m_stopInetTrafficAction->isChecked());

    int i = 0;
    for (AppGroup *appGroup : m_firewallConf->appGroupsList()) {
        const QAction *action = m_appGroupActions.at(i);
        appGroup->setEnabled(action->isChecked());
        ++i;
    }

    m_fortSettings->writeConfIni(*m_firewallConf);

    updateDriverConf(m_firewallConf, true);
}

FirewallConf *FortManager::cloneConf(const FirewallConf &conf)
{
    auto newConf = new FirewallConf(this);

    const QVariant data = conf.toVariant();
    newConf->fromVariant(data);

    newConf->copyFlags(conf);

    return newConf;
}

void FortManager::saveWindowState()
{
    m_fortSettings->setWindowGeometry(m_appWindowState->geometry());
    m_fortSettings->setWindowMaximized(m_appWindowState->maximized());

    emit afterSaveWindowState();
}

void FortManager::restoreWindowState()
{
    m_appWindowState->restore(m_appWindow, QSize(1024, 768),
                              m_fortSettings->windowGeometry(),
                              m_fortSettings->windowMaximized());

    emit afterRestoreWindowState();
}

void FortManager::saveGraphWindowState(bool visible)
{
    m_fortSettings->setGraphWindowVisible(visible);
    m_fortSettings->setGraphWindowGeometry(m_graphWindowState->geometry());
    m_fortSettings->setGraphWindowMaximized(m_graphWindowState->maximized());
}

void FortManager::restoreGraphWindowState()
{
    m_graphWindowState->restore(m_graphWindow, QSize(400, 300),
                                m_fortSettings->graphWindowGeometry(),
                                m_fortSettings->graphWindowMaximized());
}

void FortManager::updateLogger()
{
    Logger *logger = Logger::instance();

    logger->setDebug(m_fortSettings->debug());
    logger->setConsole(m_fortSettings->console());
}

void FortManager::updateTrayMenu()
{
    const FirewallConf &conf = *m_firewallConf;
    const bool hotKeyEnabled = fortSettings()->hotKeyEnabled();

    QMenu *menu = m_trayIcon->contextMenu();
    if (menu) {
        menu->deleteLater();
    }

    menu = new QMenu(&m_window);

    QAction *optionsAction = addAction(
                menu, QIcon(":/images/cog.png"), tr("Options"),
                this, SLOT(showWindow()));
    addHotKey(optionsAction, fortSettings()->hotKeyOptions(), hotKeyEnabled);

    m_graphWindowAction = addAction(
                menu, QIcon(":/images/chart_bar.png"), tr("Traffic Graph"),
                this, SLOT(switchGraphWindow()), true,
                (m_graphWindow != nullptr));
    addHotKey(m_graphWindowAction, fortSettings()->hotKeyGraph(),
              conf.logStat());

    if (!fortSettings()->hasPassword() && !m_firewallConfToEdit) {
        menu->addSeparator();

        m_filterEnabledAction = addAction(
                    menu, QIcon(), tr("Filter Enabled"),
                    this, SLOT(saveTrayFlags()),
                    true, conf.filterEnabled());
        addHotKey(m_filterEnabledAction,
                  fortSettings()->hotKeyFilter(), hotKeyEnabled);

        m_stopTrafficAction = addAction(
                    menu, QIcon(), tr("Stop Traffic"),
                    this, SLOT(saveTrayFlags()),
                    true, conf.stopTraffic());
        addHotKey(m_stopTrafficAction,
                  fortSettings()->hotKeyStopTraffic(), hotKeyEnabled);

        m_stopInetTrafficAction = addAction(
                    menu, QIcon(), tr("Stop Internet Traffic"),
                    this, SLOT(saveTrayFlags()),
                    true, conf.stopInetTraffic());
        addHotKey(m_stopInetTrafficAction,
                  fortSettings()->hotKeyStopInetTraffic(), hotKeyEnabled);

        menu->addSeparator();
        m_appGroupActions.clear();
        int appGroupIndex = 0;
        for (const AppGroup *appGroup : conf.appGroupsList()) {
            QAction *a = addAction(
                        menu, QIcon(":/images/application_double.png"),
                        appGroup->menuLabel(), this, SLOT(saveTrayFlags()),
                        true, appGroup->enabled());

            const QString shortcutText =
                    fortSettings()->hotKeyAppGroupModifiers()
                    + "+F" + QString::number(++appGroupIndex);

            addHotKey(a, shortcutText, hotKeyEnabled);

            m_appGroupActions.append(a);
        }
    }

    if (!fortSettings()->hasPassword()) {
        menu->addSeparator();
        QAction *quitAction = addAction(
                    menu, QIcon(":/images/cross.png"), tr("Quit"),
                    this, SLOT(exit()));
        addHotKey(quitAction, fortSettings()->hotKeyQuit(), hotKeyEnabled);
    }

    m_trayIcon->setContextMenu(menu);
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
