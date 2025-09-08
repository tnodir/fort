#ifndef TRAYICON_TYPES_H
#define TRAYICON_TYPES_H

namespace tray {

enum ClickType : qint8 {
    SingleClick = 0,
    CtrlSingleClick,
    AltSingleClick,
    DoubleClick,
    MiddleClick,
    RightClick,
    ClickTypeCount
};

enum ActionType : qint8 {
    ActionNone = -1,
    ActionShowHome = 0,
    ActionShowPrograms,
    ActionShowProgramsOrAlert,
    ActionShowOptions,
    ActionShowStatistics,
    ActionShowTrafficGraph,
    ActionSwitchFilterEnabled,
    ActionSwitchSnoozeAlerts,
    ActionShowBlockTrafficMenu,
    ActionShowFilterModeMenu,
    ActionShowTrayMenu,
    ActionIgnore,
    ActionTypeCount
};

enum MessageType : qint8 {
    MessageOptions,
    MessageNewVersion,
    MessageZones,
    MessageAlert,
};

}

#endif // TRAYICON_TYPES_H
