#include "speedlimit.h"

#include <util/formatutil.h>

bool SpeedLimit::isOptionsEqual(const SpeedLimit &o) const
{
    return enabled == o.enabled && inbound == o.inbound && packetLoss == o.packetLoss
            && latency == o.latency && kbps == o.kbps && bufferSize == o.bufferSize
            && notes == o.notes;
}

bool SpeedLimit::isNameEqual(const SpeedLimit &o) const
{
    return name == o.name;
}

QString SpeedLimit::menuLabel() const
{
    constexpr QChar inChar(0x2193); // ↓
    constexpr QChar outChar(0x2191); // ↑

    return name + QLatin1Char(' ') + (inbound ? inChar : outChar)
            + FormatUtil::formatSpeed(qint64(kbps) * 1024);
}
