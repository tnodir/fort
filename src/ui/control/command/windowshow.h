#ifndef WINDOWSHOW_H
#define WINDOWSHOW_H

#include <QString>

#include <form/form_types.h>

struct WindowShow
{
    bool activate : 1 = true;
    bool expose : 1 = false;
    WindowCode code;
    const char *name;

    static const WindowShow *byName(const QString &name);

    static QString namesText();
};

// As by the "home show" and "graph show": the main window exposed, the graph not activated
inline constexpr WindowShow windowShows[] = {
    { .expose = true, .code = WindowMain, .name = "main" },
    { .expose = true, .code = WindowMain, .name = "home" }, // COMPAT: v3.21.1
    { .code = WindowPrograms, .name = "programs" },
    { .code = WindowOptions, .name = "options" },
    { .code = WindowRules, .name = "rules" },
    { .code = WindowZones, .name = "zones" },
    { .code = WindowGroups, .name = "groups" },
    { .code = WindowSpeedLimits, .name = "speed-limits" },
    { .code = WindowTimePeriods, .name = "time-periods" },
    { .code = WindowConnections, .name = "connections" },
    { .code = WindowTraffic, .name = "statistics" },
    { .code = WindowServices, .name = "services" },
    { .code = WindowFilterSim, .name = "filter-sim" },
    { .activate = false, .code = WindowGraph, .name = "graph" },
};

#endif // WINDOWSHOW_H
