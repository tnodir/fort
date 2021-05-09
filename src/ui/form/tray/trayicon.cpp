#include "trayicon.h"

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include "../../conf/addressgroup.h"
#include "../../conf/appgroup.h"
#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortcompat.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../util/guiutil.h"
#include "../../util/hotkeymanager.h"
#include "../../util/iconcache.h"
#include "../controls/mainwindow.h"
#include "traycontroller.h"

namespace {

void setActionCheckable(QAction *action, bool checked = false, const QObject *receiver = nullptr,
        const char *member = nullptr)
{
    action->setCheckable(true);
    action->setChecked(checked);

    if (receiver) {
        QObject::connect(action, SIGNAL(toggled(bool)), receiver, member);
    }
}

QAction *addAction(QWidget *widget, const QIcon &icon, const QString &text,
        const QObject *receiver = nullptr, const char *member = nullptr, bool checkable = false,
        bool checked = false)
{
    auto action = new QAction(icon, text, widget);

    if (receiver) {
        QObject::connect(action, SIGNAL(triggered(bool)), receiver, member);
    }
    if (checkable) {
        setActionCheckable(action, checked);
    }

    widget->addAction(action);

    return action;
}

}

TrayIcon::TrayIcon(FortManager *fortManager, QObject *parent) :
    QSystemTrayIcon(parent), m_ctrl(new TrayController(fortManager, this))
{
    setupUi();
    setupController();
}

FortManager *TrayIcon::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *TrayIcon::settings() const
{
    return ctrl()->settings();
}

ConfManager *TrayIcon::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *TrayIcon::conf() const
{
    return ctrl()->conf();
}

IniOptions *TrayIcon::ini() const
{
    return ctrl()->ini();
}

HotKeyManager *TrayIcon::hotKeyManager() const
{
    return ctrl()->hotKeyManager();
}

void TrayIcon::updateTrayIcon(bool alerted)
{
    const auto icon = alerted
            ? GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/sign-warning.png")
            : IconCache::icon(":/icons/sheild-96.png");

    this->setIcon(icon);
}

void TrayIcon::showTrayMenu(QMouseEvent *event)
{
    QMenu *menu = this->contextMenu();
    if (!menu)
        return;

    menu->popup(mouseEventGlobalPos(event));
}

void TrayIcon::updateTrayMenu(bool onlyFlags)
{
    if (!onlyFlags) {
        updateAppGroupActions();
    }

    updateTrayMenuFlags();
    updateHotKeys();
}

void TrayIcon::updateTrayMenuFlags()
{
    const bool editEnabled = (!settings()->isPasswordRequired() && !fortManager()->optWindow());

    m_filterEnabledAction->setEnabled(editEnabled);
    m_stopTrafficAction->setEnabled(editEnabled);
    m_stopInetTrafficAction->setEnabled(editEnabled);
    m_allowAllNewAction->setEnabled(editEnabled);

    m_filterEnabledAction->setChecked(conf()->filterEnabled());
    m_stopTrafficAction->setChecked(conf()->stopTraffic());
    m_stopInetTrafficAction->setChecked(conf()->stopInetTraffic());
    m_allowAllNewAction->setChecked(conf()->allowAllNew());

    int appGroupIndex = 0;
    for (QAction *action : qAsConst(m_appGroupActions)) {
        if (!action->isVisible())
            break;

        const bool appGroupEnabled = conf()->appGroupEnabled(appGroupIndex++);

        action->setEnabled(editEnabled);
        action->setChecked(appGroupEnabled);
    }
}

void TrayIcon::setupController()
{
    connect(fortManager(), &FortManager::optWindowChanged, this, &TrayIcon::updateTrayMenuFlags);
    connect(fortManager(), &FortManager::graphWindowChanged, m_graphWindowAction,
            &QAction::setChecked);

    connect(settings(), &FortSettings::passwordCheckedChanged, this,
            &TrayIcon::updateTrayMenuFlags);

    connect(ctrl(), &TrayController::retranslateUi, this, &TrayIcon::retranslateUi);

    retranslateUi();
}

void TrayIcon::retranslateUi()
{
    m_programsAction->setText(tr("Programs"));
    m_optionsAction->setText(tr("Options"));
    m_zonesAction->setText(tr("Zones"));
    m_graphWindowAction->setText(tr("Traffic Graph"));
    m_connectionsAction->setText(tr("Connections"));

    m_filterEnabledAction->setText(tr("Filter Enabled"));
    m_stopTrafficAction->setText(tr("Stop Traffic"));
    m_stopInetTrafficAction->setText(tr("Stop Internet Traffic"));
    m_allowAllNewAction->setText(tr("Auto-Allow New Programs"));

    m_quitAction->setText(tr("Quit"));
}

void TrayIcon::setupUi()
{
    this->setToolTip(QApplication::applicationDisplayName());

    setupTrayMenu();
    updateAppGroupActions();
    updateTrayMenuFlags();
    updateHotKeys();

    updateTrayIcon();
}

void TrayIcon::setupTrayMenu()
{
    QMenu *menu = new QMenu(fortManager()->mainWindow());

    m_programsAction = addAction(menu, IconCache::icon(":/icons/window.png"), QString(),
            fortManager(), SLOT(showProgramsWindow()));
    addHotKey(m_programsAction, ini()->hotKeyPrograms());

    m_optionsAction = addAction(menu, IconCache::icon(":/icons/cog.png"), QString(), fortManager(),
            SLOT(showOptionsWindow()));
    addHotKey(m_optionsAction, ini()->hotKeyOptions());

    m_zonesAction = addAction(menu, IconCache::icon(":/icons/map-map-marker.png"), QString(),
            fortManager(), SLOT(showZonesWindow()));
    addHotKey(m_zonesAction, ini()->hotKeyZones());

    m_graphWindowAction = addAction(menu, IconCache::icon(":/icons/line-graph.png"), QString(),
            fortManager(), SLOT(switchGraphWindow()), true, !!fortManager()->graphWindow());
    addHotKey(m_graphWindowAction, ini()->hotKeyGraph());

    m_connectionsAction = addAction(menu, IconCache::icon(":/icons/connect.png"), QString(),
            fortManager(), SLOT(showConnectionsWindow()));
    addHotKey(m_connectionsAction, ini()->hotKeyConnections());

    menu->addSeparator();

    m_filterEnabledAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_filterEnabledAction, ini()->hotKeyFilter());

    m_stopTrafficAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopTrafficAction, ini()->hotKeyStopTraffic());

    m_stopInetTrafficAction =
            addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopInetTrafficAction, ini()->hotKeyStopInetTraffic());

    m_allowAllNewAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_allowAllNewAction, ini()->hotKeyAllowAllNew());

    menu->addSeparator();

    for (int i = 0; i < MAX_APP_GROUP_COUNT; ++i) {
        QAction *a = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);

        if (i < 12) {
            const QString shortcutText =
                    ini()->hotKeyAppGroupModifiers() + "+F" + QString::number(i + 1);

            addHotKey(a, shortcutText);
        }

        m_appGroupActions.append(a);
    }

    menu->addSeparator();
    m_quitAction = addAction(menu, QIcon(), tr("Quit"), fortManager(), SLOT(quitByCheckPassword()));
    addHotKey(m_quitAction, ini()->hotKeyQuit());

    this->setContextMenu(menu);
}

void TrayIcon::updateAppGroupActions()
{
    const int appGroupsCount = conf()->appGroups().count();

    for (int i = 0; i < MAX_APP_GROUP_COUNT; ++i) {
        QAction *action = m_appGroupActions.at(i);
        QString menuLabel;
        bool visible = false;

        if (i < appGroupsCount) {
            const AppGroup *appGroup = conf()->appGroups().at(i);
            menuLabel = appGroup->menuLabel();
            visible = true;
        }

        action->setText(menuLabel);
        action->setVisible(visible);
        action->setEnabled(visible);
    }
}

void TrayIcon::saveTrayFlags()
{
    conf()->setFilterEnabled(m_filterEnabledAction->isChecked());
    conf()->setStopTraffic(m_stopTrafficAction->isChecked());
    conf()->setStopInetTraffic(m_stopInetTrafficAction->isChecked());
    conf()->setAllowAllNew(m_allowAllNewAction->isChecked());

    int i = 0;
    for (AppGroup *appGroup : conf()->appGroups()) {
        const QAction *action = m_appGroupActions.at(i++);
        appGroup->setEnabled(action->isChecked());
    }

    confManager()->saveFlags();
}

void TrayIcon::addHotKey(QAction *action, const QString &shortcutText)
{
    if (shortcutText.isEmpty())
        return;

    const QKeySequence shortcut = QKeySequence::fromString(shortcutText);
    hotKeyManager()->addAction(action, shortcut);
}

void TrayIcon::updateHotKeys()
{
    hotKeyManager()->setEnabled(conf()->ini().hotKeyEnabled());
}

void TrayIcon::removeHotKeys()
{
    hotKeyManager()->removeActions();
}
