#include "trayicon.h"

#include <QApplication>
#include <QMenu>
#include <QTimer>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/mainwindow.h>
#include <fortsettings.h>
#include <manager/hotkeymanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

#include "traycontroller.h"

namespace {

const char *const eventSingleClick = "singleClick";
const char *const eventDoubleClick = "doubleClick";
const char *const eventMiddleClick = "middleClick";

const char *const actionShowPrograms = "Programs";
const char *const actionShowOptions = "Options";
const char *const actionShowStatistics = "Statistics";
const char *const actionShowTrafficGraph = "TrafficGraph";
const char *const actionSwitchFilterEnabled = "FilterEnabled";
const char *const actionSwitchStopTraffic = "StopTraffic";
const char *const actionSwitchStopInetTraffic = "StopInetTraffic";
const char *const actionSwitchAutoAllowPrograms = "AutoAllowPrograms";

QString clickNameByType(TrayIcon::ClickType clickType)
{
    switch (clickType) {
    case TrayIcon::SingleClick:
        return eventSingleClick;
    case TrayIcon::DoubleClick:
        return eventDoubleClick;
    case TrayIcon::MiddleClick:
        return eventMiddleClick;
    default:
        return QString();
    }
}

QString actionNameByType(TrayIcon::ActionType actionType)
{
    switch (actionType) {
    case TrayIcon::ActionShowPrograms:
        return actionShowPrograms;
    case TrayIcon::ActionShowOptions:
        return actionShowOptions;
    case TrayIcon::ActionShowStatistics:
        return actionShowStatistics;
    case TrayIcon::ActionShowTrafficGraph:
        return actionShowTrafficGraph;
    case TrayIcon::ActionSwitchFilterEnabled:
        return actionSwitchFilterEnabled;
    case TrayIcon::ActionSwitchStopTraffic:
        return actionSwitchStopTraffic;
    case TrayIcon::ActionSwitchStopInetTraffic:
        return actionSwitchStopInetTraffic;
    case TrayIcon::ActionSwitchAutoAllowPrograms:
        return actionSwitchAutoAllowPrograms;
    default:
        return QString();
    }
}

TrayIcon::ActionType actionTypeByName(const QString &name)
{
    if (name.isEmpty())
        return TrayIcon::ActionNone;

    if (name == actionShowPrograms)
        return TrayIcon::ActionShowPrograms;

    if (name == actionShowOptions)
        return TrayIcon::ActionShowOptions;

    if (name == actionShowStatistics)
        return TrayIcon::ActionShowStatistics;

    if (name == actionShowTrafficGraph)
        return TrayIcon::ActionShowTrafficGraph;

    if (name == actionSwitchFilterEnabled)
        return TrayIcon::ActionSwitchFilterEnabled;

    if (name == actionSwitchStopTraffic)
        return TrayIcon::ActionSwitchStopTraffic;

    if (name == actionSwitchStopInetTraffic)
        return TrayIcon::ActionSwitchStopInetTraffic;

    if (name == actionSwitchAutoAllowPrograms)
        return TrayIcon::ActionSwitchAutoAllowPrograms;

    return TrayIcon::ActionNone;
}

TrayIcon::ActionType defaultActionTypeByClick(TrayIcon::ClickType clickType)
{
    switch (clickType) {
    case TrayIcon::SingleClick:
        return TrayIcon::ActionShowPrograms;
    case TrayIcon::DoubleClick:
        return TrayIcon::ActionShowOptions;
    case TrayIcon::MiddleClick:
        return TrayIcon::ActionShowStatistics;
    default:
        return TrayIcon::ActionNone;
    }
}

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

TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent), m_trayTriggered(false), m_ctrl(new TrayController(this))
{
    setupUi();
    setupController();

    connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::onTrayActivated);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
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

WindowManager *TrayIcon::windowManager() const
{
    return ctrl()->windowManager();
}

void TrayIcon::onMouseClicked(TrayIcon::ClickType clickType)
{
    QAction *action = m_clickActions[clickType];
    if (action) {
        action->trigger();
    }
}

void TrayIcon::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        m_trayTriggered = true;
        QTimer::singleShot(QApplication::doubleClickInterval(), this, [&] {
            if (m_trayTriggered) {
                m_trayTriggered = false;
                onMouseClicked(SingleClick);
            }
        });
        break;
    case QSystemTrayIcon::DoubleClick:
        if (m_trayTriggered) {
            m_trayTriggered = false;
            onMouseClicked(DoubleClick);
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        m_trayTriggered = false;
        onMouseClicked(MiddleClick);
        break;
    case QSystemTrayIcon::Context:
        m_trayTriggered = false;
        showTrayMenu(QCursor::pos());
        break;
    default:
        break;
    }
}

void TrayIcon::updateTrayIcon(bool alerted)
{
    const auto icon = alerted ? GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/error.png")
                              : IconCache::icon(":/icons/sheild-96.png");

    this->setIcon(icon);
}

void TrayIcon::showTrayMenu(const QPoint &pos)
{
    m_menu->popup(pos);
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
    connect(windowManager(), &WindowManager::optWindowChanged, this,
            &TrayIcon::updateTrayMenuFlags);
    connect(windowManager(), &WindowManager::graphWindowChanged, m_graphAction,
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
    m_statisticsAction->setText(tr("Statistics"));
    m_zonesAction->setText(tr("Zones"));
    m_graphAction->setText(tr("Traffic Graph"));

    m_filterEnabledAction->setText(tr("Filter Enabled"));
    m_stopTrafficAction->setText(tr("Stop Traffic"));
    m_stopInetTrafficAction->setText(tr("Stop Internet Traffic"));
    m_autoAllowProgsAction->setText(tr("Auto-Allow New Programs"));

    m_quitAction->setText(tr("Quit"));
}

void TrayIcon::setupUi()
{
    this->setToolTip(QApplication::applicationDisplayName());

    setupTrayMenu();
    updateTrayMenu();

    updateTrayIcon();

    updateClickActions();
}

void TrayIcon::setupTrayMenu()
{
    m_menu = new QMenu();

    m_programsAction = addAction(m_menu, IconCache::icon(":/icons/application.png"), QString(),
            windowManager(), SLOT(showProgramsWindow()));
    addHotKey(m_programsAction, iniUser()->hotKeyPrograms());

    m_optionsAction = addAction(m_menu, IconCache::icon(":/icons/cog.png"), QString(),
            windowManager(), SLOT(showOptionsWindow()));
    addHotKey(m_optionsAction, iniUser()->hotKeyOptions());

    m_statisticsAction = addAction(m_menu, IconCache::icon(":/icons/chart_bar.png"), QString(),
            windowManager(), SLOT(showStatisticsWindow()));
    addHotKey(m_statisticsAction, iniUser()->hotKeyStatistics());

    m_graphAction = addAction(m_menu, IconCache::icon(":/icons/action_log.png"), QString(),
            windowManager(), SLOT(switchGraphWindow()), true, !!windowManager()->graphWindow());
    addHotKey(m_graphAction, iniUser()->hotKeyGraph());

    m_zonesAction = addAction(m_menu, IconCache::icon(":/icons/ip_class.png"), QString(),
            windowManager(), SLOT(showZonesWindow()));
    addHotKey(m_zonesAction, iniUser()->hotKeyZones());

    m_menu->addSeparator();

    m_filterEnabledAction =
            addAction(m_menu, QIcon(), QString(), this, SLOT(switchTrayFlag(bool)), true);
    addHotKey(m_filterEnabledAction, iniUser()->hotKeyFilter());

    m_stopTrafficAction =
            addAction(m_menu, QIcon(), QString(), this, SLOT(switchTrayFlag(bool)), true);
    addHotKey(m_stopTrafficAction, iniUser()->hotKeyStopTraffic());

    m_stopInetTrafficAction =
            addAction(m_menu, QIcon(), QString(), this, SLOT(switchTrayFlag(bool)), true);
    addHotKey(m_stopInetTrafficAction, iniUser()->hotKeyStopInetTraffic());

    m_autoAllowProgsAction =
            addAction(m_menu, QIcon(), QString(), this, SLOT(switchTrayFlag(bool)), true);
    addHotKey(m_autoAllowProgsAction, iniUser()->hotKeyAllowAllNew());

    m_menu->addSeparator();

    for (int i = 0; i < MAX_APP_GROUP_COUNT; ++i) {
        QAction *a = addAction(m_menu, QIcon(), QString(), this, SLOT(switchTrayFlag(bool)), true);

        if (i < 12) {
            const QString shortcutText =
                    iniUser()->hotKeyAppGroupModifiers() + "+F" + QString::number(i + 1);

            addHotKey(a, shortcutText);
        }

        m_appGroupActions.append(a);
    }

    m_menu->addSeparator();
    m_quitAction = addAction(m_menu, QIcon(), tr("Quit"), this, SLOT(quitProgram()));
    addHotKey(m_quitAction, iniUser()->hotKeyQuit());
}

void TrayIcon::updateTrayMenuFlags()
{
    const bool editEnabled = (!settings()->isPasswordRequired() && !windowManager()->optWindow());

    m_filterEnabledAction->setEnabled(editEnabled);
    m_stopTrafficAction->setEnabled(editEnabled);
    m_stopInetTrafficAction->setEnabled(editEnabled);
    m_autoAllowProgsAction->setEnabled(editEnabled);

    m_filterEnabledAction->setChecked(conf()->filterEnabled());
    m_stopTrafficAction->setChecked(conf()->stopTraffic());
    m_stopInetTrafficAction->setChecked(conf()->stopInetTraffic());
    m_autoAllowProgsAction->setChecked(conf()->allowAllNew());

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
    conf()->setAllowAllNew(m_autoAllowProgsAction->isChecked());

    int i = 0;
    for (AppGroup *appGroup : conf()->appGroups()) {
        const QAction *action = m_appGroupActions.at(i++);
        appGroup->setEnabled(action->isChecked());
    }

    confManager()->saveFlags();
}

void TrayIcon::switchTrayFlag(bool checked)
{
    if (iniUser()->confirmTrayFlags()) {
        const auto action = qobject_cast<QAction *>(sender());
        Q_ASSERT(action);

        if (!windowManager()->showQuestionBox(
                    tr("Are you sure to switch the \"%1\"?").arg(action->text()))) {
            action->setChecked(!checked);
            return;
        }
    }

    saveTrayFlags();
}

void TrayIcon::quitProgram()
{
    if (iniUser()->confirmQuit()) {
        if (!windowManager()->showQuestionBox(tr("Are you sure you want to quit the program?")))
            return;
    }

    windowManager()->quitByCheckPassword();
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

TrayIcon::ActionType TrayIcon::clickEventActionType(ClickType clickType) const
{
    const QString eventName = clickNameByType(clickType);
    const QString actionName = iniUser()->trayAction(eventName);

    const ActionType actionType = actionTypeByName(actionName);
    return (actionType != ActionNone) ? actionType : defaultActionTypeByClick(clickType);
}

void TrayIcon::setClickEventActionType(ClickType clickType, ActionType actionType)
{
    const QString eventName = clickNameByType(clickType);
    const QString actionName = actionNameByType(actionType);

    iniUser()->setTrayAction(eventName, actionName);

    updateClickActions();
}

void TrayIcon::updateClickActions()
{
    m_clickActions[SingleClick] = clickActionFromIni(SingleClick);
    m_clickActions[DoubleClick] = clickActionFromIni(DoubleClick);
    m_clickActions[MiddleClick] = clickActionFromIni(MiddleClick);
}

QAction *TrayIcon::clickActionFromIni(ClickType clickType) const
{
    const ActionType actionType = clickEventActionType(clickType);

    return clickActionByType(actionType);
}

QAction *TrayIcon::clickActionByType(ActionType actionType) const
{
    switch (actionType) {
    case TrayIcon::ActionShowPrograms:
        return m_programsAction;
    case TrayIcon::ActionShowOptions:
        return m_optionsAction;
    case TrayIcon::ActionShowStatistics:
        return m_statisticsAction;
    case TrayIcon::ActionShowTrafficGraph:
        return m_graphAction;
    case TrayIcon::ActionSwitchFilterEnabled:
        return m_filterEnabledAction;
    case TrayIcon::ActionSwitchStopTraffic:
        return m_stopTrafficAction;
    case TrayIcon::ActionSwitchStopInetTraffic:
        return m_stopInetTrafficAction;
    case TrayIcon::ActionSwitchAutoAllowPrograms:
        return m_autoAllowProgsAction;
    }

    return nullptr;
}
