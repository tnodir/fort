#include "fortmanager.h"

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QWindow>

#include "activityLog/logbuffer.h"
#include "activityLog/logentry.h"
#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "driver/drivermanager.h"
#include "fortsettings.h"
#include "util/fileutil.h"
#include "util/hostinfo.h"
#include "util/netutil.h"
#include "util/osutil.h"

FortManager::FortManager(QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_engine(nullptr),
    m_appWindow(nullptr),
    m_fortSettings(new FortSettings(qApp->arguments(), this)),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullConf()),
    m_driverManager(new DriverManager(this))
{
    setupDriver();

    loadSettings(m_firewallConf);

    registerQmlTypes();

    setupTrayIcon();
}

void FortManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<DriverManager>("com.fortfirewall", 1, 0, "DriverManager",
                                             "Singleton");
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterType<AddressGroup>("com.fortfirewall", 1, 0, "AddressGroup");
    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");

    qmlRegisterType<LogBuffer>("com.fortfirewall", 1, 0, "LogBuffer");
    qmlRegisterType<LogEntry>("com.fortfirewall", 1, 0, "LogEntry");

    qmlRegisterType<FileUtil>("com.fortfirewall", 1, 0, "FileUtil");
    qmlRegisterType<HostInfo>("com.fortfirewall", 1, 0, "HostInfo");
    qmlRegisterType<NetUtil>("com.fortfirewall", 1, 0, "NetUtil");
    qmlRegisterType<OsUtil>("com.fortfirewall", 1, 0, "OsUtil");
}

bool FortManager::setupDriver()
{
    if (!m_driverManager->openDevice()) {
        showErrorBox(m_driverManager->errorMessage());
        return false;
    }

    return true;
}

void FortManager::setupTrayIcon()
{
    m_trayIcon->setToolTip(qApp->applicationDisplayName());
    m_trayIcon->setIcon(QIcon(":/images/shield.png"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            showWindow();
    });

    updateTrayMenu();
}

void FortManager::setupEngine()
{
    m_engine = new QQmlApplicationEngine(this);

    m_engine->rootContext()->setContextProperty("fortManager", this);

    m_engine->load(QUrl("qrc:/qml/main.qml"));

    m_appWindow = qobject_cast<QWindow *>(
                m_engine->rootObjects().first());
    Q_ASSERT(m_appWindow);
}

void FortManager::showTrayIcon()
{
    m_trayIcon->show();
}

void FortManager::showWindow()
{
    if (!m_engine) {
        setupEngine();
    }

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

    if (m_engine) {
        m_engine->deleteLater();
        m_engine = nullptr;
        m_appWindow = nullptr;
    }

    qApp->exit(retcode);
}

void FortManager::showErrorBox(const QString &text)
{
    QMessageBox::critical(&m_window, QString(), text);
}

bool FortManager::saveConf(bool onlyFlags)
{
    return saveSettings(m_firewallConfToEdit, onlyFlags);
}

bool FortManager::applyConf(bool onlyFlags)
{
    Q_ASSERT(m_firewallConfToEdit != nullConf());

    FirewallConf *newConf = cloneConf(*m_firewallConfToEdit);

    newConf->copyTempFlags(*m_firewallConf);

    return saveSettings(newConf, onlyFlags);
}

void FortManager::setFirewallConfToEdit(FirewallConf *conf)
{
    if (m_firewallConfToEdit != nullConf()
            && m_firewallConfToEdit != m_firewallConf) {
        m_firewallConfToEdit->deleteLater();
    }

    m_firewallConfToEdit = conf;
    emit firewallConfToEditChanged();
}

bool FortManager::loadSettings(FirewallConf *conf)
{
    if (!m_fortSettings->readConf(*conf)) {
        showErrorBox(m_fortSettings->errorMessage());
        return false;
    }

    return updateDriverConf(conf);
}

bool FortManager::saveSettings(FirewallConf *newConf, bool onlyFlags)
{
    if (!(onlyFlags ? m_fortSettings->writeConfFlags(*newConf)
          : m_fortSettings->writeConf(*newConf))) {
        showErrorBox(m_fortSettings->errorMessage());
        return false;
    }

    m_firewallConf->deleteLater();
    m_firewallConf = newConf;

    updateTrayMenu();

    return onlyFlags ? updateDriverConfFlags(m_firewallConf)
                     : updateDriverConf(m_firewallConf);
}

bool FortManager::updateDriverConf(FirewallConf *conf)
{
    // Update driver
    if (!m_driverManager->writeConf(*conf)) {
        showErrorBox(m_driverManager->errorMessage());
        return false;
    }

    return true;
}

bool FortManager::updateDriverConfFlags(FirewallConf *conf)
{
    // Update driver
    if (!m_driverManager->writeConfFlags(*conf)) {
        showErrorBox(m_driverManager->errorMessage());
        return false;
    }

    return true;
}

void FortManager::setAppLogBlocked(bool enable)
{
    m_firewallConf->setAppLogBlocked(enable);

    updateDriverConfFlags(m_firewallConf);
}

void FortManager::saveTrayFlags()
{
    m_firewallConf->setFilterEnabled(m_filterEnabledAction->isChecked());
    m_firewallConf->ipInclude()->setUseAll(m_ipIncludeAllAction->isChecked());
    m_firewallConf->ipExclude()->setUseAll(m_ipExcludeAllAction->isChecked());
    m_firewallConf->setAppBlockAll(m_appBlockAllAction->isChecked());
    m_firewallConf->setAppAllowAll(m_appAllowAllAction->isChecked());

    int i = 0;
    foreach (AppGroup *appGroup, m_firewallConf->appGroupsList()) {
        const QAction *action = m_appGroupActions.at(i);
        appGroup->setEnabled(action->isChecked());
        ++i;
    }

    m_fortSettings->writeConfFlags(*m_firewallConf);

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

    menu->addSeparator();
    m_filterEnabledAction = addAction(
                menu, QIcon(), tr("Filter Enabled"),
                this, SLOT(saveTrayFlags()),
                true, conf.filterEnabled());

    menu->addSeparator();
    m_ipIncludeAllAction = addAction(
                menu, QIcon(), tr("Include All Addresses"),
                this, SLOT(saveTrayFlags()),
                true, conf.ipInclude()->useAll());
    m_ipExcludeAllAction = addAction(
                menu, QIcon(), tr("Exclude All Addresses"),
                this, SLOT(saveTrayFlags()),
                true, conf.ipExclude()->useAll());

    menu->addSeparator();
    m_appBlockAllAction = addAction(
                menu, QIcon(), tr("Block All Applications"),
                this, SLOT(saveTrayFlags()),
                true, conf.appBlockAll());
    m_appAllowAllAction = addAction(
                menu, QIcon(), tr("Allow All Applications"),
                this, SLOT(saveTrayFlags()),
                true, conf.appAllowAll());

    menu->addSeparator();
    foreach (const AppGroup *appGroup, conf.appGroupsList()) {
        QAction *a = addAction(
                    menu, QIcon(":/images/application_double.png"),
                    appGroup->name(), this, SLOT(saveTrayFlags()),
                    true, appGroup->enabled());
        m_appGroupActions.append(a);
    }

    menu->addSeparator();
    addAction(menu, QIcon(":/images/cancel.png"), tr("Quit"),
              this, SLOT(exit()));

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
