#include "ip4range.h"

#include <QHash>
#include <QRegularExpression>

#include "netutil.h"

Ip4Range::Ip4Range(QObject *parent) :
    QObject(parent)
{
}

void Ip4Range::clear()
{
    m_errorLineNo = 0;
    m_errorMessage = QString();

    m_ipArray.clear();
    m_pairFromArray.clear();
    m_pairToArray.clear();
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

QString Ip4Range::errorLineAndMessage() const
{
    return tr("Error at line %1: %2")
            .arg(QString::number(m_errorLineNo),
                 m_errorMessage);
}

QString Ip4Range::toText() const
{
    QString text;

    for (int i = 0, n = ipSize(); i < n; ++i) {
        const quint32 ip = ipAt(i);

        text += QString("%1\n")
                .arg(NetUtil::ip4ToText(ip));
    }

    for (int i = 0, n = pairSize(); i < n; ++i) {
        const Ip4Pair ip = pairAt(i);

        text += QString("%1-%2\n")
                .arg(NetUtil::ip4ToText(ip.from),
                     NetUtil::ip4ToText(ip.to));
    }

    return text;
}

bool Ip4Range::fromText(const QString &text)
{
    const auto list = text.splitRef(QLatin1Char('\n'));
    return fromList(list);
}

bool Ip4Range::fromList(const QVector<QStringRef> &list, int emptyNetMask, bool sort)
{
    Q_UNUSED(sort)  // TODO

    clear();

    ip4range_map_t ipRangeMap;
    int pairSize = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty()
                || lineTrimmed.startsWith('#'))  // commented line
            continue;

        quint32 from = 0, to = 0;
        if (!parseAddressMask(lineTrimmed, from, to, emptyNetMask)) {
            setErrorLineNo(lineNo);
            return false;
        }

        ipRangeMap.insert(from, to);

        if (from != to) {
            ++pairSize;
        }
    }

    fillRange(ipRangeMap, pairSize);

    setErrorLineNo(0);

    return true;
}

// Parse "127.0.0.0-127.255.255.255" or "127.0.0.0/24" or "127.0.0.0"
bool Ip4Range::parseAddressMask(const QStringRef &line,
                                quint32 &from, quint32 &to,
                                int emptyNetMask)
{
    const QRegularExpression re(R"(([\d.]+)\s*([/-]?)\s*(\S*))");
    const auto match = re.match(line);

    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return false;
    }

    const auto ip = match.captured(1);
    const auto sepStr = match.capturedRef(2);
    const auto sep = sepStr.isEmpty() ? QChar('/') : sepStr.at(0);
    const auto mask = match.captured(3);

    if (sepStr.isEmpty() != mask.isEmpty()) {
        setErrorMessage(tr("Bad mask"));
        return false;
    }

    bool ok;

    from = NetUtil::textToIp4(ip, &ok);
    if (!ok) {
        setErrorMessage(tr("Bad IP address"));
        return false;
    }

    if (sep == QLatin1Char('-')) {  // e.g. "127.0.0.0-127.255.255.255"
        to = NetUtil::textToIp4(mask, &ok);
        if (!ok) {
            setErrorMessage(tr("Bad second IP address"));
            return false;
        }
        if (from > to) {
            setErrorMessage(tr("Bad range"));
            return false;
        }
    } else if (sep == QLatin1Char('/')) {  // e.g. "127.0.0.0/24", "127.0.0.0"
        bool ok = true;
        const int nbits = mask.isEmpty() ? emptyNetMask : mask.toInt(&ok);

        if (!ok || nbits < 0 || nbits > 32) {
            setErrorMessage(tr("Bad mask"));
            return false;
        }

        to = from | (nbits == 32 ? 0 : ((1 << (32 - nbits)) - 1));
    }

    return true;
}

void Ip4Range::fillRange(const ip4range_map_t &ipRangeMap, int pairSize)
{
    ip4range_map_t::const_iterator it = ipRangeMap.constBegin();
    ip4range_map_t::const_iterator end = ipRangeMap.constEnd();

    const int mapSize = ipRangeMap.size();
    m_ipArray.reserve(mapSize - pairSize);
    m_pairFromArray.reserve(pairSize);
    m_pairToArray.reserve(pairSize);

    Ip4Pair prevIp;
    int prevIndex = -1;

    for (; it != end; ++it) {
        Ip4Pair ip{it.key(), it.value()};

        // try to merge colliding addresses
        if (prevIndex >= 0 && ip.from <= prevIp.to + 1) {
            if (ip.to > prevIp.to) {
                m_pairToArray.replace(prevIndex, ip.to);

                prevIp.to = ip.to;
            }
            // else skip it
        } else if (ip.from == ip.to) {
            m_ipArray.append(ip.from);
        } else {
            m_pairFromArray.append(ip.from);
            m_pairToArray.append(ip.to);

            prevIp = ip;
            ++prevIndex;
        }
    }
}
