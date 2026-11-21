#ifndef FORM_TYPES_H
#define FORM_TYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowNone = 0,
    WindowMain = (1 << 0),
    WindowPrograms = (1 << 1),
    WindowProgramAlert = (1 << 2),
    WindowServices = (1 << 3),
    WindowOptions = (1 << 4),
    WindowRules = (1 << 5),
    WindowTraffic = (1 << 6),
    WindowActivity = (1 << 7),
    WindowConnections = (1 << 8),
    WindowZones = (1 << 9),
    WindowGroups = (1 << 10),
    WindowSpeedLimits = (1 << 11),
    WindowTimePeriods = (1 << 12),
    WindowFilterSim = (1 << 13),
    WindowGraph = (1 << 14),
    WindowInstallation = (1 << 15),
    WindowUpdates = (1 << 16),
    WindowCount = 17,
    WindowPasswordDialog = (1 << 17),
};

constexpr quint32 WindowPasswordProtected =
        (WindowPrograms | WindowProgramAlert | WindowServices | WindowOptions | WindowRules
                | WindowTraffic | WindowConnections | WindowZones | WindowGroups | WindowSpeedLimits
                | WindowTimePeriods | WindowFilterSim | WindowInstallation | WindowUpdates
                | WindowActivity);

/* The windows, which keep the password unlocked while they are open: the Main window is shown
 * without the password, its docked windows are disabled while the password is required */
constexpr quint32 WindowPasswordUnlocking = (WindowMain | WindowPasswordProtected);

/* The windows, which can be docked into the Main window */
constexpr quint32 WindowDockable = (WindowPrograms | WindowServices | WindowRules | WindowTraffic
        | WindowConnections | WindowZones | WindowGroups | WindowSpeedLimits | WindowTimePeriods
        | WindowActivity);

#endif // FORM_TYPES_H
