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
    bool inbound : 1 = false;

    quint8 limitId = 0;

    quint16 packetLoss = 0; // in 1/100 percent

    quint32 latency = 0; // milliseconds
    quint32 kbps = 0; // kilobits per second
    quint32 bufferSize = DefaultSpeedLimitBufferSize; // bytes

    QString name;
    QString notes;

    QDateTime modTime;
};

#endif // SPEEDLIMIT_H
