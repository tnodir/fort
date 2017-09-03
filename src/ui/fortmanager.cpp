#include "fortmanager.h"

#include <QApplication>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>
#include <QWindow>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortsettings.h"
#include "util/fileutil.h"

FortManager::FortManager(QObject *parent) :
    QObject(parent),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_engine(new QQmlApplicationEngine(this)),
    m_fortSettings(new FortSettings(qApp->arguments(), this)),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullConf())
{
    m_fortSettings->readConf(*m_firewallConf);

    registerQmlTypes();

    setupTrayIcon();
    setupEngine();
}

bool FortManager::startWithWindows() const
{
    return FileUtil::fileExists(startupShortcutPath());
}

void FortManager::setStartWithWindows(bool start)
{
    const QString linkPath = startupShortcutPath();
    if (start) {
        FileUtil::linkFile(qApp->applicationFilePath(), linkPath);
    } else {
        FileUtil::removeFile(linkPath);
    }
    emit startWithWindowsChanged();
}

void FortManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<FortManager>("com.fortfirewall", 1, 0, "FortManager",
                                            "Singleton");
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterType<AddressGroup>("com.fortfirewall", 1, 0, "AddressGroup");
    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");
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

bool FortManager::saveConf()
{
    return saveSettings(m_firewallConfToEdit);
}

bool FortManager::applyConf()
{
    Q_ASSERT(m_firewallConfToEdit != nullConf());
    return saveSettings(cloneConf(*m_firewallConfToEdit));
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
    if (!m_fortSettings->writeConf(*newConf))
        return false;

    m_firewallConf->deleteLater();
    m_firewallConf = newConf;

    updateTrayMenu();

    return true;
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
    QMenu *menu = m_trayIcon->contextMenu();
    if (menu) {
        menu->deleteLater();
    }

    menu = new QMenu(&m_window);

    addAction(menu, QIcon(), tr("Show"), this, SLOT(showWindow()));

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

QString FortManager::startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\')
            + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}
