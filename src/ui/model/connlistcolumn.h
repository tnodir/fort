#ifndef CONNLISTCOLUMN_H
#define CONNLISTCOLUMN_H

enum class ConnListColumn : qint8 {
    Program = 0,
    ProcessId,
    Protocol,
    LocalHostName,
    LocalIp,
    LocalPort,
    RemoteHostName,
    RemoteIp,
    RemotePort,
    Direction,
    Action,
    Reason,
    Time,
    Count,
};

#endif // CONNLISTCOLUMN_H
