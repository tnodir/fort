#ifndef WINDOWTYPES_H
#define WINDOWTYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowHome = (1 << 0),
    WindowPrograms = (1 << 1),
    WindowServices = (1 << 2),
    WindowOptions = (1 << 3),
    WindowPolicies = (1 << 4),
    WindowStatistics = (1 << 5),
    WindowZones = (1 << 6),
    WindowGraph = (1 << 7),
    WindowPasswordDialog = (1 << 8),
};

constexpr quint32 WindowPasswordProtected = (WindowPrograms | WindowServices | WindowOptions
        | WindowPolicies | WindowStatistics | WindowZones);

#endif // WINDOWTYPES_H
