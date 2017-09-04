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

FortManager::FortManager(QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_engine(new QQmlApplicationEngine(this)),
    m_fortSettings(new FortSettings(qApp->arguments(), this)),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullConf()),
    m_driverManager(new DriverManager(this))
{
    m_fortSettings->readConf(*m_firewallConf);

    registerQmlTypes();

    setupTrayIcon();
    setupEngine();
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
    m_engine->rootContext()->setContextProperty("fortManager", this);

    m_engine->load(QUrl("qrc:/qml/main.qml"));

    m_appWindow = qobject_cast<QWindow *>(
                m_engine->rootObjects().first());
    Q_ASSERT(m_appWindow);
}

bool FortManager::setupDriver()
{
    if (!m_driverManager->openDevice()) {
        showErrorBox(m_driverManager->errorMessage());
        return false;
    }

    return true;
}

void FortManager::showTrayIcon()
{
    m_trayIcon->show();
}

void FortManager::showWindow()
{
    if (m_firewallConfToEdit == nullConf()) {
        setFirewallConfToEdit(cloneConf(*m_firewallConf));
    }

    m_appWindow->show();
    m_appWindow->raise();
    m_appWindow->requestActivate();
}

void FortManager::closeWindow()
{
    m_appWindow->hide();

    setFirewallConfToEdit(nullConf());
}

void FortManager::showErrorBox(const QString &text)
{
    QMessageBox::critical(&m_window, QString(), text);
}

bool FortManager::saveConf()
{
    return saveSettings(m_firewallConfToEdit);
}

bool FortManager::applyConf()
{
    Q_ASSERT(m_firewallConfToEdit != nullConf());

    FirewallConf *newConf = cloneConf(*m_firewallConfToEdit);

    newConf->copyTempFlags(*m_firewallConf);

    return saveSettings(newConf);
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

bool FortManager::saveSettings(FirewallConf *newConf)
{
    if (!m_fortSettings->writeConf(*newConf)) {
        showErrorBox(m_fortSettings->errorMessage());
        return false;
    }

    m_firewallConf->deleteLater();
    m_firewallConf = newConf;

    updateTrayMenu();

    // Update driver
    if (!m_driverManager->writeConf(*m_firewallConf)) {
        showErrorBox(m_driverManager->errorMessage());
        return false;
    }

    return true;
}

bool FortManager::updateDriverFlags(FirewallConf *conf)
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

    updateDriverFlags(m_firewallConf);
}

void FortManager::saveTrayFlags()
{
    m_firewallConf->setFilterEnabled(m_filterEnabledAction->isChecked());
    m_firewallConf->ipInclude()->setUseAll(m_ipIncludeAllAction->isChecked());
    m_firewallConf->ipExclude()->setUseAll(m_ipExcludeAllAction->isChecked());
    m_firewallConf->setAppBlockAll(m_appBlockAllAction->isChecked());
    m_firewallConf->setAppAllowAll(m_appAllowAllAction->isChecked());

    int i = 0;
    const QList<QAction*> groupActions = m_appGroupsMenu->actions();
    foreach (AppGroup *appGroup, m_firewallConf->appGroupsList()) {
        const QAction *action = groupActions.at(i);
        appGroup->setEnabled(action->isChecked());
        ++i;
    }

    m_fortSettings->writeConfFlags(*m_firewallConf);

    updateDriverFlags(m_firewallConf);
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

    addAction(menu, QIcon(), tr("Show"), this, SLOT(showWindow()));

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
    m_appGroupsMenu = new QMenu(tr("Application Groups"), menu);
    menu->addMenu(m_appGroupsMenu);

    foreach (const AppGroup *appGroup, conf.appGroupsList()) {
        addAction(m_appGroupsMenu, QIcon(), appGroup->name(),
                  this, SLOT(saveTrayFlags()),
                  true, appGroup->enabled());
    }

    menu->addSeparator();
    addAction(menu, QIcon(), tr("Quit"), qApp, SLOT(quit()));

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
