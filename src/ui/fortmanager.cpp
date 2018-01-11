#include "fortmanager.h"

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWindow>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "db/databasemanager.h"
#include "driver/drivermanager.h"
#include "fortsettings.h"
#include "log/logmanager.h"
#include "log/model/appblockedmodel.h"
#include "log/model/appstatmodel.h"
#include "log/model/iplistmodel.h"
#include "log/model/traflistmodel.h"
#include "task/taskinfo.h"
#include "task/taskmanager.h"
#include "translationmanager.h"
#include "util/fileutil.h"
#include "util/guiutil.h"
#include "util/logger.h"
#include "util/net/hostinfocache.h"
#include "util/net/netutil.h"
#include "util/osutil.h"
#include "util/stringutil.h"
#include "util/windowstatewatcher.h"

FortManager::FortManager(FortSettings *fortSettings,
                         QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_engine(nullptr),
    m_appWindow(nullptr),
    m_appWindowState(new WindowStateWatcher(this)),
    m_fortSettings(fortSettings),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullConf()),
    m_databaseManager(new DatabaseManager(fortSettings->statFilePath(), this)),
    m_driverManager(new DriverManager(this)),
    m_logManager(new LogManager(m_databaseManager,
                                m_driverManager->driverWorker(), this)),
    m_taskManager(new TaskManager(this, this))
{
    setupDriver();
    setupLogManager();

    loadSettings(m_firewallConf);

    setupLogger();

    m_taskManager->loadSettings(m_fortSettings);

    TranslationManager::instance()->switchLanguageByName(
                m_fortSettings->language());

    registerQmlTypes();

    setupTrayIcon();
}

FortManager::~FortManager()
{
    closeDriver();
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

    qmlRegisterType<FileUtil>("com.fortfirewall", 1, 0, "FileUtil");
    qmlRegisterType<GuiUtil>("com.fortfirewall", 1, 0, "GuiUtil");
    qmlRegisterType<HostInfoCache>("com.fortfirewall", 1, 0, "HostInfoCache");
    qmlRegisterType<NetUtil>("com.fortfirewall", 1, 0, "NetUtil");
    qmlRegisterType<OsUtil>("com.fortfirewall", 1, 0, "OsUtil");
    qmlRegisterType<StringUtil>("com.fortfirewall", 1, 0, "StringUtil");
}

bool FortManager::setupDriver()
{
    if (!m_driverManager->openDevice()) {
        showErrorBox("Setup Driver: " + m_driverManager->errorMessage());
        return false;
    }

    return true;
}

void FortManager::closeDriver()
{
    m_logManager->setActive(false);

    m_driverManager->closeDevice();
}

void FortManager::setupLogger()
{
    Logger::instance()->setPath(m_fortSettings->logsPath());

    updateLogger();
}

void FortManager::setupLogManager()
{
    m_databaseManager->initialize();

    m_logManager->initialize();
    m_logManager->setActive(true);
}

void FortManager::setupTrayIcon()
{
    m_trayIcon->setToolTip(qApp->applicationDisplayName());
    m_trayIcon->setIcon(QIcon(":/images/shield.png"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showWindow();
        }
    });

    updateTrayMenu();
}

bool FortManager::setupEngine()
{
    m_engine = new QQmlApplicationEngine(this);

    QQmlContext *context = m_engine->rootContext();
    context->setContextProperty("fortManager", this);
    context->setContextProperty("translationManager", TranslationManager::instance());

    m_engine->load(QUrl("qrc:/qml/main.qml"));

    if (m_engine->rootObjects().isEmpty()) {
        showErrorBox("Cannot setup QML Engine");
        return false;
    }

    m_appWindow = qobject_cast<QWindow *>(
                m_engine->rootObjects().first());
    Q_ASSERT(m_appWindow);

    m_appWindowState->setup(m_appWindow);

    // XXX: Workaround to fix icons' incorrect position on main tab buttons
    QTimer::singleShot(100, this, &FortManager::restoreWindowState);

    return true;
}

void FortManager::showTrayIcon()
{
    m_trayIcon->show();
}

void FortManager::showTrayMessage(const QString &message)
{
    m_trayIcon->showMessage(qApp->applicationDisplayName(), message);
}

void FortManager::showWindow()
{
    if (!m_engine) {
        setupEngine();
    }

    if (!m_appWindow) return;

    if (m_firewallConfToEdit == nullConf()) {
        setFirewallConfToEdit(cloneConf(*m_firewallConf));
    }

    m_appWindow->show();
    m_appWindow->raise();
    m_appWindow->requestActivate();
}

void FortManager::closeWindow()
{
    if (m_appWindow) {
        m_appWindow->hide();
    }

    setFirewallConfToEdit(nullConf());
}

void FortManager::exit(int retcode)
{
    closeWindow();

    if (m_appWindow) {
        saveWindowState();
        m_appWindow = nullptr;
    }

    if (m_engine) {
        m_engine->deleteLater();
        m_engine = nullptr;
    }

    qApp->exit(retcode);
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

    return onlyFlags ? updateDriverConfFlags(m_firewallConf)
                     : updateDriverConf(m_firewallConf);
}

bool FortManager::updateDriverConf(FirewallConf *conf)
{
    if (!m_driverManager->isDeviceOpened())
        return true;

    // Update driver
    if (!m_driverManager->writeConf(*conf)) {
        showErrorBox("Update Driver Conf: " + m_driverManager->errorMessage());
        return false;
    }

    updateDatabaseManager(conf);

    return true;
}

bool FortManager::updateDriverConfFlags(FirewallConf *conf)
{
    if (!m_driverManager->isDeviceOpened())
        return true;

    // Update driver
    if (!m_driverManager->writeConfFlags(*conf)) {
        showErrorBox("Update Driver Conf Flags: " + m_driverManager->errorMessage());
        return false;
    }

    updateDatabaseManager(conf);

    return true;
}

void FortManager::updateDatabaseManager(FirewallConf *conf)
{
    m_databaseManager->setFirewallConf(conf);
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

    int i = 0;
    foreach (AppGroup *appGroup, m_firewallConf->appGroupsList()) {
        const QAction *action = m_appGroupActions.at(i);
        appGroup->setEnabled(action->isChecked());
        ++i;
    }

    m_fortSettings->writeConfIni(*m_firewallConf);

    updateDriverConfFlags(m_firewallConf);
}

FirewallConf *FortManager::cloneConf(const FirewallConf &conf)
{
    FirewallConf *newConf = new FirewallConf(this);

    const QVariant data = conf.toVariant();
    newConf->fromVariant(data);

    newConf->copyFlags(conf);

    return newConf;
}

void FortManager::saveWindowState()
{
    m_fortSettings->setWindowGeometry(m_appWindowState->geometry());
    m_fortSettings->setWindowMaximized(m_appWindowState->maximized());
}

void FortManager::restoreWindowState()
{
    const QRect rect = m_fortSettings->windowGeometry();

    if (rect.isNull()) {
        m_appWindow->resize(1024, 768);
        return;
    }

    const bool maximized = m_fortSettings->windowMaximized();

    m_appWindowState->setGeometry(rect);
    m_appWindowState->setMaximized(maximized);

    m_appWindow->setGeometry(rect);

    if (maximized) {
        m_appWindow->setVisibility(QWindow::Maximized);
    }
}

void FortManager::updateLogger()
{
    Logger::setupLogging(m_firewallConf->logErrors(),
                         m_fortSettings->debug(),
                         m_fortSettings->console());
}

void FortManager::updateTrayMenu()
{
    const FirewallConf &conf = *m_firewallConf;

    QMenu *menu = m_trayIcon->contextMenu();
    if (menu) {
        menu->deleteLater();
    }

    menu = new QMenu(&m_window);

    addAction(menu, QIcon(":/images/cog.png"), tr("Options"),
              this, SLOT(showWindow()));

    if (!conf.hasPassword() && !m_firewallConfToEdit) {
        menu->addSeparator();
        m_filterEnabledAction = addAction(
                    menu, QIcon(), tr("Filter Enabled"),
                    this, SLOT(saveTrayFlags()),
                    true, conf.filterEnabled());
        m_stopTrafficAction = addAction(
                    menu, QIcon(), tr("Stop Traffic"),
                    this, SLOT(saveTrayFlags()),
                    true, conf.stopTraffic());

        menu->addSeparator();
        m_appGroupActions.clear();
        foreach (const AppGroup *appGroup, conf.appGroupsList()) {
            QAction *a = addAction(
                        menu, QIcon(":/images/application_double.png"),
                        appGroup->name(), this, SLOT(saveTrayFlags()),
                        true, appGroup->enabled());
            m_appGroupActions.append(a);
        }
    }

    if (!conf.hasPassword()) {
        menu->addSeparator();
        addAction(menu, QIcon(":/images/cross.png"), tr("Quit"),
                  this, SLOT(exit()));
    }

    m_trayIcon->setContextMenu(menu);
}

QAction *FortManager::addAction(QWidget *widget,
                                const QIcon &icon, const QString &text,
                                const QObject *receiver, const char *member,
                                bool checkable, bool checked)
{
    QAction *action = new QAction(icon, text, widget);

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
