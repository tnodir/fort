#ifndef SPEEDLIMIT_H
#define SPEEDLIMIT_H

#include <QDateTime>
#include <QObject>

inline constexpr int DefaultSpeedLimitBufferSize = 150000;

class SpeedLimit
{
public:
    bool isOptionsEqual(const SpeedLimit &o) const;
    bool isNameEqual(const SpeedLimit &o) const;

    QString menuLabel() const;

public:
    bool enabled : 1 = true;
    bool inbound : 1 = true;

    quint8 limitId = 0;

    quint16 packetLoss = 0; // in 1/100 percent

    quint32 latency = 0; // milliseconds
    quint32 kbps = 0; // kilobits per second
    quint32 bufferSize = DefaultSpeedLimitBufferSize; // bytes

    QString name;
    QString notes;

    QDateTime modTime;
};

struct SpeedLimitStatus
{
    bool isValid = false; // the driver has the Speed Limit's queue

    quint64 queuedBytes = 0;
    quint64 droppedCount = 0; // on the buffer's overflow
    quint64 lostCount = 0; // by the packet loss rate
};

#endif // SPEEDLIMIT_H
