#include "speedlimit.h"

#include <util/formatutil.h>

bool SpeedLimit::isNameEqual(const SpeedLimit &o) const
{
    return name == o.name;
}

bool SpeedLimit::isOptionsEqual(const SpeedLimit &o) const
{
    return enabled == o.enabled && inbound == o.inbound && packetLoss == o.packetLoss
            && latency == o.latency && kbps == o.kbps && bufferSize == o.bufferSize;
}

QString SpeedLimit::menuLabel() const
{
    QString text = name;

    if (enabledKbps() != 0) {
        constexpr QChar inChar(0x2193); // ↓
        constexpr QChar outChar(0x2191); // ↑

        text += QLatin1Char(' ') + (inbound ? inChar : outChar)
                + FormatUtil::formatSpeed(enabledKbps() * 1024LL);
    }

    return text;
}
