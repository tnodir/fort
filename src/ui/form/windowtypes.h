#ifndef WINDOWTYPES_H
#define WINDOWTYPES_H

#include <QtGlobal>

enum WindowCode {
    WindowHome = (1 << 0),
    WindowPrograms = (1 << 1),
    WindowProgramAlert = (1 << 2),
    WindowServices = (1 << 3),
    WindowOptions = (1 << 4),
    WindowRules = (1 << 5),
    WindowStatistics = (1 << 6),
    WindowZones = (1 << 7),
    WindowGraph = (1 << 8),
    WindowPasswordDialog = (1 << 9),
};

constexpr quint32 WindowPasswordProtected = (WindowPrograms | WindowProgramAlert | WindowServices
        | WindowOptions | WindowRules | WindowStatistics | WindowZones);

#endif // WINDOWTYPES_H
