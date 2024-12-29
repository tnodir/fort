#ifndef APPLISTCOLUMN_H
#define APPLISTCOLUMN_H

enum class AppListColumn : qint8 {
    Name = 0,
    Zones,
    Rule,
    Scheduled,
    Action,
    Group,
    FilePath,
    CreationTime,
    Count
};

#endif // APPLISTCOLUMN_H
