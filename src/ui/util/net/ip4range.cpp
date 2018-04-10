#include "ip4range.h"

#include <QHash>
#include <QRegularExpression>

#include "netutil.h"

Ip4Range::Ip4Range(QObject *parent) :
    QObject(parent),
    m_errorLineNo(0)
{
}

void Ip4Range::clear()
{
    m_errorLineNo = 0;
    m_errorMessage = QString();

    m_fromArray.clear();
    m_toArray.clear();
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

QString Ip4Range::toText()
{
    QString text;

    const int n = size();
    for (int i = 0; i < n; ++i) {
        const Ip4Pair ip = at(i);

        text += QString("%1-%2\n")
                .arg(NetUtil::ip4ToText(ip.from),
                     NetUtil::ip4ToText(ip.to));
    }

    return text;
}

bool Ip4Range::fromText(const QString &text)
{
    clear();

    ip4range_map_t ipRangeMap;

    int lineNo = 0;
    foreach (const QStringRef &line,
             text.splitRef(QLatin1Char('\n'))) {
        ++lineNo;

        const QStringRef lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty()
                || lineTrimmed.startsWith('#'))  // commented line
            continue;

        quint32 from, to;
        if (!parseAddressMask(lineTrimmed, from, to)) {
            setErrorLineNo(lineNo);
            return false;
        }

        ipRangeMap.insert(from, to);
    }

    fillRange(ipRangeMap);

    setErrorLineNo(0);

    return true;
}

// Parse "127.0.0.0-127.255.255.255" or "127.0.0.0/24" or "127.0.0.0"
bool Ip4Range::parseAddressMask(const QStringRef &line,
                                quint32 &from, quint32 &to)
{
    const QRegularExpression re(R"(([\d.]+)\s*([/-]?)\s*(\S*))");
    const QRegularExpressionMatch match = re.match(line);

    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return false;
    }

    const QString ip = match.captured(1);
    const QString sepStr = match.captured(2);
    const QChar sep = sepStr.isEmpty() ? QLatin1Char('/') : sepStr.at(0);
    const QString mask = match.captured(3);

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
        const int nbits = mask.isEmpty() ? 24 : mask.toInt(&ok);

        if (!ok || nbits < 0 || nbits > 32) {
            setErrorMessage(tr("Bad mask"));
            return false;
        }

        to = from | (nbits == 32 ? 0 : ((1 << (32 - nbits)) - 1));
    }

    return true;
}

void Ip4Range::fillRange(const ip4range_map_t &ipRangeMap)
{
    ip4range_map_t::const_iterator it = ipRangeMap.constBegin();
    ip4range_map_t::const_iterator end = ipRangeMap.constEnd();

    const int mapSize = ipRangeMap.size();
    m_fromArray.reserve(mapSize);
    m_toArray.reserve(mapSize);

    Ip4Pair prevIp;
    int prevIndex = -1;

    for (; it != end; ++it) {
        Ip4Pair ip{it.key(), it.value()};

        // try to merge colliding addresses
        if (prevIndex >= 0 && ip.from <= prevIp.to + 1) {
            if (ip.to > prevIp.to) {
                m_toArray.replace(prevIndex, ip.to);

                prevIp.to = ip.to;
            }
            // else skip it
        } else {
            m_fromArray.append(ip.from);
            m_toArray.append(ip.to);

            prevIp = ip;
            ++prevIndex;
        }
    }
}
