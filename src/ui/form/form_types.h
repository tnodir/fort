#ifndef FORM_TYPES_H
#define FORM_TYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowNone = 0,
    WindowHome = (1 << 0),
    WindowPrograms = (1 << 1),
    WindowProgramAlert = (1 << 2),
    WindowGroups = (1 << 3),
    WindowSpeedLimits = (1 << 4),
    WindowServices = (1 << 5),
    WindowOptions = (1 << 6),
    WindowRules = (1 << 7),
    WindowStatistics = (1 << 8),
    WindowZones = (1 << 9),
    WindowGraph = (1 << 10),
    WindowPasswordDialog = (1 << 11),
};

constexpr quint32 WindowPasswordProtected =
        (WindowPrograms | WindowProgramAlert | WindowGroups | WindowSpeedLimits | WindowServices
                | WindowOptions | WindowRules | WindowStatistics | WindowZones);

#endif // FORM_TYPES_H
