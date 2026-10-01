#ifndef APPLISTCOLUMN_H
#define APPLISTCOLUMN_H

enum class AppListColumn : qint8 {
    Name = 0,
    Zones,
    Rule,
    Groups,
    SpeedLimits,
    Scheduled,
    Action,
    FilePath,
    CreationTime,
    Notes,
    Count,
};

#endif // APPLISTCOLUMN_H
