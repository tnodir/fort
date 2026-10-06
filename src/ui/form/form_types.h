#ifndef FORM_TYPES_H
#define FORM_TYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowNone = 0,
    WindowHome = (1 << 0),
    WindowPrograms = (1 << 1),
    WindowProgramAlert = (1 << 2),
    WindowServices = (1 << 3),
    WindowOptions = (1 << 4),
    WindowRules = (1 << 5),
    WindowTraffic = (1 << 6),
    WindowConnections = (1 << 7),
    WindowZones = (1 << 8),
    WindowGroups = (1 << 9),
    WindowSpeedLimits = (1 << 10),
    WindowTimePeriods = (1 << 11),
    WindowFilterSim = (1 << 12),
    WindowGraph = (1 << 13),
    WindowCount = 14,
    WindowPasswordDialog = (1 << 14),
};

constexpr quint32 WindowPasswordProtected = (WindowPrograms | WindowProgramAlert | WindowServices
        | WindowOptions | WindowRules | WindowTraffic | WindowConnections | WindowZones
        | WindowGroups | WindowSpeedLimits | WindowTimePeriods | WindowFilterSim);

#endif // FORM_TYPES_H
