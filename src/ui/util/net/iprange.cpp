#include "iprange.h"

#include <QHash>
#include <QRegularExpression>

#include <util/stringutil.h>

#include "netutil.h"

IpRange::IpRange(QObject *parent) : QObject(parent) { }

void IpRange::clear()
{
    m_errorLineNo = 0;
    m_errorMessage.clear();

    m_ip4Array.clear();
    m_pair4FromArray.clear();
    m_pair4ToArray.clear();

    m_ip6Array.clear();
    m_pair6FromArray.clear();
    m_pair6ToArray.clear();
}

void IpRange::setErrorLineNo(int lineNo)
{
    if (m_errorLineNo != lineNo) {
        m_errorLineNo = lineNo;
        emit errorLineNoChanged();
    }
}

void IpRange::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

QString IpRange::errorLineAndMessage() const
{
    return tr("Error at line %1: %2").arg(QString::number(m_errorLineNo), m_errorMessage);
}

bool IpRange::isEmpty() const
{
    return ip4Size() == 0 && pair4Size() == 0 && ip6Size() == 0 && pair6Size() == 0;
}

QString IpRange::toText() const
{
    QString text;

    for (int i = 0, n = ip4Size(); i < n; ++i) {
        const quint32 ip = ip4At(i);

        text += QString("%1\n").arg(NetUtil::ip4ToText(ip));
    }

    for (int i = 0, n = pair4Size(); i < n; ++i) {
        const Ip4Pair ip = pair4At(i);

        text += QString("%1-%2\n").arg(NetUtil::ip4ToText(ip.from), NetUtil::ip4ToText(ip.to));
    }

    for (int i = 0, n = ip6Size(); i < n; ++i) {
        const ip6_addr_t ip = ip6At(i);

        text += QString("%1\n").arg(NetUtil::ip6ToText(ip));
    }

    for (int i = 0, n = pair6Size(); i < n; ++i) {
        const Ip6Pair ip = pair6At(i);

        text += QString("%1-%2\n").arg(NetUtil::ip6ToText(ip.from), NetUtil::ip6ToText(ip.to));
    }

    return text;
}

bool IpRange::fromText(const QString &text)
{
    const auto list = StringUtil::splitView(text, QLatin1Char('\n'));
    return fromList(list);
}

bool IpRange::fromList(const StringViewList &list, int emptyNetMask, bool sort)
{
    Q_UNUSED(sort); // TODO: Sort parsed IP addresses

    clear();

    ip4range_map_t ip4RangeMap;
    int pair4Size = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        const bool isIPv4 = lineTrimmed.contains('.');
        if (isIPv4) {
            quint32 fromIp4 = 0, toIp4 = 0;

            if (parseIp4AddressMask(lineTrimmed, fromIp4, toIp4, emptyNetMask) != ErrorOk) {
                setErrorLineNo(lineNo);
                return false;
            }

            ip4RangeMap.insert(fromIp4, toIp4);

            if (fromIp4 != toIp4) {
                ++pair4Size;
            }
        } else {
            bool hasIp6Mask = false;
            ip6_addr_t fromIp6, toIp6;

            if (parseIp6AddressMask(lineTrimmed, fromIp6, toIp6, hasIp6Mask) != ErrorOk) {
                setErrorLineNo(lineNo);
                return false;
            }

            if (hasIp6Mask) {
                m_pair6FromArray.append(fromIp6);
                m_pair6ToArray.append(toIp6);
            } else {
                m_ip6Array.append(fromIp6);
            }
        }
    }

    fillIp4Range(ip4RangeMap, pair4Size);

    setErrorLineNo(0);

    return true;
}

// Parse "127.0.0.0-127.255.255.255" or "127.0.0.0/24" or "127.0.0.0"
IpRange::ParseError IpRange::parseIp4AddressMask(
        const StringView line, quint32 &from, quint32 &to, int emptyNetMask)
{
    static const QRegularExpression re(R"(^([\d.]+)\s*([\/-]?)\s*(\S*))");

    const auto match = re.match(line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto ip = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto sep = sepStr.isEmpty() ? QChar('/') : sepStr.at(0);
    const auto mask = match.captured(3);

    if (sepStr.isEmpty() != mask.isEmpty()) {
        setErrorMessage(tr("Bad IPv4 mask"));
        return ErrorBadMaskFormat;
    }

    bool ok;

    from = NetUtil::textToIp4(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IPv4 address"));
        return ErrorBadAddress;
    }

    if (sep == QLatin1Char('-')) { // e.g. "127.0.0.0-127.255.255.255"
        to = NetUtil::textToIp4(mask, &ok);
        if (!ok) {
            setErrorMessage(tr("Bad second IPv4 address"));
            return ErrorBadAddress2;
        }
        if (from > to) {
            setErrorMessage(tr("Bad IPv4 range"));
            return ErrorBadRange;
        }
    } else if (sep == QLatin1Char('/')) { // e.g. "127.0.0.0/24", "127.0.0.0"
        bool ok = true;
        const int nbits = mask.isEmpty() ? emptyNetMask : mask.toInt(&ok);

        if (!ok || nbits < 0 || nbits > 32) {
            setErrorMessage(tr("Bad IPv4 mask value"));
            return ErrorBadMask;
        }

        to = nbits == 0 ? quint32(-1) : (from | (nbits == 32 ? 0 : ((1 << (32 - nbits)) - 1)));
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6AddressMask(
        const StringView line, ip6_addr_t &from, ip6_addr_t &to, bool &hasIp6Mask)
{
    static const QRegularExpression re(R"(^([A-Fa-f\d:]+)\s*([\/-]?)\s*(\S*))");

    const auto match = re.match(line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto ip = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto sep = sepStr.isEmpty() ? QChar('\0') : sepStr.at(0);
    const auto mask = match.captured(3);

    if (sepStr.isEmpty() != mask.isEmpty()) {
        setErrorMessage(tr("Bad IPv6 mask"));
        return ErrorBadMaskFormat;
    }

    bool ok;

    from = NetUtil::textToIp6(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IPv6 address"));
        return ErrorBadAddress;
    }

    hasIp6Mask = true;

    if (sep == QLatin1Char('-')) { // e.g. "::1 - ::2"
        to = NetUtil::textToIp6(mask, &ok);
        if (!ok) {
            setErrorMessage(tr("Bad second IP IPv6 address"));
            return ErrorBadAddress2;
        }
    } else if (sep == QLatin1Char('/')) { // e.g. "::1/24", "::1"
        bool ok = true;
        const int nbits = mask.toInt(&ok);

        if (!ok || nbits < 0 || nbits > 128) {
            setErrorMessage(tr("Bad IPv6 mask value"));
            return ErrorBadMask;
        }

        if (nbits == 128) {
            hasIp6Mask = false;
            return ErrorOk;
        }

        const int maskBits = 128 - nbits;
        const int maskBytes = maskBits / 8;
        const int remBits = maskBits % 8;
        const int off = sizeof(ip6_addr_t) - maskBytes;

        memcpy(&to.data[0], &from.data[0], off);
        memset(&to.data[off], -1, maskBytes);
        if (remBits != 0) {
            to.data[off - 1] |= (1 << remBits) - 1;
        }
    } else {
        hasIp6Mask = false;
    }

    return ErrorOk;
}

void IpRange::fillIp4Range(const ip4range_map_t &ipRangeMap, int pairSize)
{
    if (ipRangeMap.isEmpty())
        return;

    const int mapSize = ipRangeMap.size();
    m_ip4Array.reserve(mapSize - pairSize);
    m_pair4FromArray.reserve(pairSize);
    m_pair4ToArray.reserve(pairSize);

    Ip4Pair prevIp;
    int prevIndex = -1;

    auto it = ipRangeMap.constBegin();
    auto end = ipRangeMap.constEnd();

    for (; it != end; ++it) {
        Ip4Pair ip { it.key(), it.value() };

        // try to merge colliding addresses
        if (prevIndex >= 0 && ip.from <= prevIp.to + 1) {
            if (ip.to > prevIp.to) {
                m_pair4ToArray.replace(prevIndex, ip.to);

                prevIp.to = ip.to;
            }
            // else skip it
        } else if (ip.from == ip.to) {
            m_ip4Array.append(ip.from);
        } else {
            m_pair4FromArray.append(ip.from);
            m_pair4ToArray.append(ip.to);

            prevIp = ip;
            ++prevIndex;
        }
    }
}
