#ifndef FORM_TYPES_H
#define FORM_TYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowNone = 0,
    WindowMain = (1 << 0),
    WindowHome = (1 << 1),
    WindowPrograms = (1 << 2),
    WindowProgramAlert = (1 << 3),
    WindowServices = (1 << 4),
    WindowOptions = (1 << 5),
    WindowRules = (1 << 6),
    WindowTraffic = (1 << 7),
    WindowConnections = (1 << 8),
    WindowZones = (1 << 9),
    WindowGroups = (1 << 10),
    WindowSpeedLimits = (1 << 11),
    WindowTimePeriods = (1 << 12),
    WindowFilterSim = (1 << 13),
    WindowGraph = (1 << 14),
    WindowCount = 15,
    WindowPasswordDialog = (1 << 15),
};

constexpr quint32 WindowPasswordProtected = (WindowMain | WindowPrograms | WindowProgramAlert
        | WindowServices | WindowOptions | WindowRules | WindowTraffic | WindowConnections
        | WindowZones | WindowGroups | WindowSpeedLimits | WindowTimePeriods | WindowFilterSim);

/* The windows, which can be docked into the Main window */
constexpr quint32 WindowDockable = (WindowPrograms | WindowServices | WindowRules | WindowTraffic
        | WindowConnections | WindowZones | WindowGroups | WindowSpeedLimits | WindowTimePeriods);

#endif // FORM_TYPES_H
