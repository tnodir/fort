#ifndef SPEEDLIMIT_H
#define SPEEDLIMIT_H

#include <QObject>

constexpr int DefaultLimitBufferSize = 150000;

class SpeedLimit
{
public:
    bool isNameEqual(const SpeedLimit &o) const;
    bool isOptionsEqual(const SpeedLimit &o) const;

    bool isNull() const { return limitId == 0; }

    quint32 enabledKbps() const { return enabled ? kbps : 0; }

    QString menuLabel() const;

public:
    bool enabled : 1 = true;
    bool inbound : 1 = false;

    quint16 packetLoss = 0; // percent
    quint32 latency = 0; // milliseconds

    quint32 kbps = 0; // kilobits per second

    quint32 bufferSize = DefaultLimitBufferSize;

    qint64 limitId = 0;

    QString name;
};

#endif // SPEEDLIMIT_H
