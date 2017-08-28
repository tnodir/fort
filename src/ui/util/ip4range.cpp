#include "ip4range.h"

#include <QHash>
#include <QRegularExpression>

#include "netutil.h"

Ip4Range::Ip4Range(QObject *parent) :
    QObject(parent),
    m_errorLineNo(0)
{
}

void Ip4Range::setErrorLineNo(int lineNo)
{
    if (m_errorLineNo != lineNo) {
        m_errorLineNo = lineNo;
        emit errorLineNoChanged();
    }
}

void Ip4Range::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

QString Ip4Range::toText()
{
    QString text;

    const int n = m_range.size();
    for (int i = 0; i < n; ++i) {
        const Ip4Pair &ip = m_range.at(i);

        text += QString("%1-%2\n")
                .arg(NetUtil::ip4ToText(ip.from),
                     NetUtil::ip4ToText(ip.to));
    }

    return text;
}

bool Ip4Range::fromText(const QString &text)
{
    m_range.clear();

    ip4range_map_t ipRangeMap;

    int lineNo = 0;
    foreach (const QStringRef &line, text.splitRef(
                 QLatin1Char('\n'), QString::SkipEmptyParts)) {
        ++lineNo;

        quint32 from, to;
        if (!parseAddressMask(line, from, to)) {
            setErrorLineNo(lineNo);
            return false;
        }

        ipRangeMap.insert(from, to);
    }

    fillRange(ipRangeMap);

    setErrorLineNo(0);

    return true;
}

// Parse "127.0.0.0-127.255.255.255" or "127.0.0.0/24"
bool Ip4Range::parseAddressMask(const QStringRef &line,
                                quint32 &from, quint32 &to)
{
    const QRegularExpression re("([\\d.]+)\\s*([/-])\\s*(\\S+)");
    const QRegularExpressionMatch match = re.match(line);

    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return false;
    }

    const QString ip = match.captured(1);
    const QChar sep = match.capturedRef(2).at(0);
    const QString mask = match.captured(3);

    bool ok;

    from = NetUtil::textToIp4(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IPv4"));
        return false;
    }

    if (sep == QLatin1Char('-')) {  // e.g. "127.0.0.0-127.255.255.255"
        to = NetUtil::textToIp4(mask, &ok);
        if (!ok) {
            setErrorMessage(tr("Bad second IPv4"));
            return false;
        }
        if (from > to) {
            setErrorMessage(tr("Bad range"));
            return false;
        }
    } else if (sep == QLatin1Char('/')) {  // e.g. "127.0.0.0/24"
        bool ok;
        const int nbits = mask.toInt(&ok);

        if (!ok || nbits < 0 || nbits > 32) {
            setErrorMessage(tr("Bad mask"));
            return false;
        }

        to = (nbits == 32) ? 0xFFFFFFFF
                           : (from | ((1 << nbits) - 1));
    }

    return true;
}

void Ip4Range::fillRange(const ip4range_map_t &ipRangeMap)
{
    ip4range_map_t::const_iterator it = ipRangeMap.constBegin();
    ip4range_map_t::const_iterator end = ipRangeMap.constEnd();

    Ip4Pair *prevIp = 0;

    m_range.reserve(ipRangeMap.size());

    for (; it != end; ++it) {
        Ip4Pair ip{it.key(), it.value()};

        // try to merge colliding adresses
        if (prevIp && ip.from <= prevIp->to) {
            if (ip.to > prevIp->to) {
                prevIp->to = ip.to;
            }
            // else skip it
        } else {
            m_range.append(ip);
            prevIp = &m_range.last();
        }
    }
}
