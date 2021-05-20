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
#include "../../user/iniuser.h"
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

IniUser *TrayIcon::iniUser() const
{
    return ctrl()->iniUser();
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

void TrayIcon::setupController()
{
    connect(fortManager(), &FortManager::optWindowChanged, this, &TrayIcon::updateTrayMenuFlags);
    connect(fortManager(), &FortManager::graphWindowChanged, m_graphAction, &QAction::setChecked);

    connect(settings(), &FortSettings::passwordCheckedChanged, this,
            &TrayIcon::updateTrayMenuFlags);

    connect(ctrl(), &TrayController::retranslateUi, this, &TrayIcon::retranslateUi);

    retranslateUi();
}

void TrayIcon::retranslateUi()
{
    m_programsAction->setText(tr("Programs"));
    m_optionsAction->setText(tr("Options"));
    m_statisticsAction->setText(tr("Statistics"));
    m_zonesAction->setText(tr("Zones"));
    m_graphAction->setText(tr("Traffic Graph"));

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
    updateTrayMenu();

    updateTrayIcon();
}

void TrayIcon::setupTrayMenu()
{
    QMenu *menu = new QMenu(fortManager()->mainWindow());

    m_programsAction = addAction(menu, IconCache::icon(":/icons/window.png"), QString(),
            fortManager(), SLOT(showProgramsWindow()));
    addHotKey(m_programsAction, iniUser()->hotKeyPrograms());

    m_optionsAction = addAction(menu, IconCache::icon(":/icons/cog.png"), QString(), fortManager(),
            SLOT(showOptionsWindow()));
    addHotKey(m_optionsAction, iniUser()->hotKeyOptions());

    m_statisticsAction = addAction(menu, IconCache::icon(":/icons/chart-bar.png"), QString(),
            fortManager(), SLOT(showStatisticsWindow()));
    addHotKey(m_statisticsAction, iniUser()->hotKeyStatistics());

    m_graphAction = addAction(menu, IconCache::icon(":/icons/line-graph.png"), QString(),
            fortManager(), SLOT(switchGraphWindow()), true, !!fortManager()->graphWindow());
    addHotKey(m_graphAction, iniUser()->hotKeyGraph());

    m_zonesAction = addAction(menu, IconCache::icon(":/icons/map-map-marker.png"), QString(),
            fortManager(), SLOT(showZonesWindow()));
    addHotKey(m_zonesAction, iniUser()->hotKeyZones());

    menu->addSeparator();

    m_filterEnabledAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_filterEnabledAction, iniUser()->hotKeyFilter());

    m_stopTrafficAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopTrafficAction, iniUser()->hotKeyStopTraffic());

    m_stopInetTrafficAction =
            addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_stopInetTrafficAction, iniUser()->hotKeyStopInetTraffic());

    m_allowAllNewAction = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);
    addHotKey(m_allowAllNewAction, iniUser()->hotKeyAllowAllNew());

    menu->addSeparator();

    for (int i = 0; i < MAX_APP_GROUP_COUNT; ++i) {
        QAction *a = addAction(menu, QIcon(), QString(), this, SLOT(saveTrayFlags()), true);

        if (i < 12) {
            const QString shortcutText =
                    iniUser()->hotKeyAppGroupModifiers() + "+F" + QString::number(i + 1);

            addHotKey(a, shortcutText);
        }

        m_appGroupActions.append(a);
    }

    menu->addSeparator();
    m_quitAction = addAction(menu, QIcon(), tr("Quit"), fortManager(), SLOT(quitByCheckPassword()));
    addHotKey(m_quitAction, iniUser()->hotKeyQuit());

    this->setContextMenu(menu);
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
    hotKeyManager()->setEnabled(iniUser()->hotKeyEnabled());
}

void TrayIcon::removeHotKeys()
{
    hotKeyManager()->removeActions();
}
