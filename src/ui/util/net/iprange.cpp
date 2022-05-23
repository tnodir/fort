#include "iprange.h"

#include <QHash>
#include <QRegularExpression>

#include <util/stringutil.h>

#include "netutil.h"

namespace {

bool compareIp6(const ip6_addr_t &l, const ip6_addr_t &r)
{
    return memcmp(&l, &r, sizeof(ip6_addr_t)) < 0;
}

void sortIp6Array(ip6_arr_t &array)
{
    std::sort(array.begin(), array.end(), compareIp6);
}

void sortIp6PairArray(ip6_arr_t &fromArray, ip6_arr_t &toArray)
{
    Q_ASSERT(fromArray.size() == toArray.size());

    const int arraySize = fromArray.size();

    ip6_pair_arr_t pairArray;
    pairArray.reserve(arraySize);

    for (int i = 0; i < arraySize; ++i) {
        pairArray.append(Ip6Pair { fromArray[i], toArray[i] });
    }

    std::sort(pairArray.begin(), pairArray.end(),
            [](const Ip6Pair &l, const Ip6Pair &r) { return compareIp6(l.from, r.from); });

    for (int i = 0; i < arraySize; ++i) {
        const Ip6Pair &pair = pairArray[i];
        fromArray[i] = pair.from;
        toArray[i] = pair.to;
    }
}

}

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
    clear();

    ip4range_map_t ip4RangeMap;
    int pair4Size = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parseIpLine(line, ip4RangeMap, pair4Size, emptyNetMask) != ErrorOk) {
            setErrorLineNo(lineNo);
            return false;
        }
    }

    fillIp4Range(ip4RangeMap, pair4Size);

    if (sort) {
        sortIp6Array(m_ip6Array);
        sortIp6PairArray(m_pair6FromArray, m_pair6ToArray);
    }

    setErrorLineNo(0);

    return true;
}

IpRange::ParseError IpRange::parseIpLine(
        const StringView line, ip4range_map_t &ip4RangeMap, int &pair4Size, int emptyNetMask)
{
    static const QRegularExpression ip4Re(R"(^([\d.]+)\s*([\/-]?)\s*(\S*))");
    static const QRegularExpression ip6Re(R"(^([A-Fa-f\d:]+)\s*([\/-]?)\s*(\S*))");

    const bool isIPv6 = !line.contains('.');
    const QRegularExpression &re = isIPv6 ? ip6Re : ip4Re;

    const auto match = re.match(line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto ip = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto mask = match.captured(3);

    if (sepStr.isEmpty() != mask.isEmpty()) {
        setErrorMessage(tr("Bad mask"));
        return ErrorBadMaskFormat;
    }

    const char maskSep = sepStr.isEmpty() ? '\0' : sepStr.at(0).toLatin1();

    return isIPv6 ? parseIp6Address(ip, mask, maskSep)
                  : parseIp4Address(ip, mask, ip4RangeMap, pair4Size, emptyNetMask, maskSep);
}

IpRange::ParseError IpRange::parseIp4Address(const QString &ip, const QString &mask,
        ip4range_map_t &ip4RangeMap, int &pair4Size, int emptyNetMask, char maskSep)
{
    quint32 from, to = 0;

    bool ok;
    from = NetUtil::textToIp4(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IPv4 address"));
        return ErrorBadAddress;
    }

    const ParseError err = parseIp4AddressMask(mask, from, to, emptyNetMask, maskSep);
    if (err != ErrorOk)
        return err;

    ip4RangeMap.insert(from, to);

    if (from != to) {
        ++pair4Size;
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp4AddressMask(
        const QString &mask, quint32 &from, quint32 &to, int emptyNetMask, char maskSep)
{
    switch (maskSep) {
    case '-': // e.g. "127.0.0.0-127.255.255.255"
        return parseIp4AddressMaskFull(mask, from, to);
    case '/':
    case '\0': // e.g. "127.0.0.0/24", "127.0.0.0"
        return parseIp4AddressMaskPrefix(mask, from, to, emptyNetMask);
    default:
        return ErrorOk;
    }
}

IpRange::ParseError IpRange::parseIp4AddressMaskFull(
        const QString &mask, quint32 &from, quint32 &to)
{
    bool ok;
    to = NetUtil::textToIp4(mask, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad second IPv4 address"));
        return ErrorBadAddress2;
    }

    if (from > to) {
        setErrorMessage(tr("Bad IPv4 range"));
        return ErrorBadRange;
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp4AddressMaskPrefix(
        const QString &mask, quint32 &from, quint32 &to, int emptyNetMask)
{
    bool ok = true;
    const int nbits = mask.isEmpty() ? emptyNetMask : mask.toInt(&ok);

    if (!ok || nbits < 0 || nbits > 32) {
        setErrorMessage(tr("Bad IPv4 mask value"));
        return ErrorBadMask;
    }

    to = NetUtil::applyIp4Mask(from, nbits);

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6Address(const QString &ip, const QString &mask, char maskSep)
{
    bool hasMask = false;
    ip6_addr_t from, to;

    bool ok;
    from = NetUtil::textToIp6(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IPv6 address"));
        return ErrorBadAddress;
    }

    const ParseError err = parseIp6AddressMask(mask, from, to, hasMask, maskSep);
    if (err != ErrorOk)
        return err;

    if (hasMask) {
        m_pair6FromArray.append(from);
        m_pair6ToArray.append(to);
    } else {
        m_ip6Array.append(from);
    }

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6AddressMask(
        const QString &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask, char maskSep)
{
    hasMask = false;

    switch (maskSep) {
    case '-': // e.g. "::1 - ::2"
        return parseIp6AddressMaskFull(mask, to, hasMask);
    case '/': // e.g. "::1/24", "::1"
        return parseIp6AddressMaskPrefix(mask, from, to, hasMask);
    default:
        return ErrorOk;
    }
}

IpRange::ParseError IpRange::parseIp6AddressMaskFull(
        const QString &mask, ip6_addr_t &to, bool &hasMask)
{
    bool ok;
    to = NetUtil::textToIp6(mask, &ok);

    if (!ok) {
        setErrorMessage(tr("Bad second IP IPv6 address"));
        return ErrorBadAddress2;
    }

    hasMask = true;

    return ErrorOk;
}

IpRange::ParseError IpRange::parseIp6AddressMaskPrefix(
        const QString &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask)
{
    bool ok;
    const int nbits = mask.toInt(&ok);

    if (!ok || nbits < 0 || nbits > 128) {
        setErrorMessage(tr("Bad IPv6 mask value"));
        return ErrorBadMask;
    }

    if (nbits == 128)
        return ErrorOk;

    to = NetUtil::applyIp6Mask(from, nbits);

    hasMask = true;

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
