#include "portrange.h"

#include <util/net/netutil.h>
#include <util/stringutil.h>

PortRange::PortRange(QObject *parent) : ValueRange(parent) { }

void PortRange::clear()
{
    ValueRange::clear();

    m_portArray.clear();
    m_pairFromArray.clear();
    m_pairToArray.clear();
}

bool PortRange::isEmpty() const
{
    return portSize() == 0 && pairSize() == 0;
}

QString PortRange::toText() const
{
    QString text;

    for (int i = 0, n = portSize(); i < n; ++i) {
        const quint16 port = portAt(i);

        text += QString("%1\n").arg(port);
    }

    for (int i = 0, n = pairSize(); i < n; ++i) {
        const PortPair pair = pairAt(i);

        text += QString("%1-%2\n").arg(QString::number(pair.from), QString::number(pair.to));
    }

    return text;
}

bool PortRange::fromText(const QString &text)
{
    const auto list = StringUtil::splitView(text, QLatin1Char('\n'));
    return fromList(list);
}

bool PortRange::fromList(const StringViewList &list)
{
    clear();

    portrange_map_t portRangeMap;
    int pairSize = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parsePortLine(line, portRangeMap, pairSize) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    fillRangeArrays<quint16>({
            .rangeMap = portRangeMap,
            .valuesArray = m_portArray,
            .pairFromArray = m_pairFromArray,
            .pairToArray = m_pairToArray,
            .pairSize = pairSize,
    });

    return true;
}

PortRange::ParseError PortRange::parsePortLine(
        const QStringView &line, portrange_map_t &portRangeMap, int &pairSize)
{
    static const QRegularExpression portRe(R"(^([\d\w]+)\s*(-?)\s*(\S*))");

    const auto match = StringUtil::match(portRe, line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto port = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto port2 = match.captured(3);

    if (sepStr.isEmpty() != port2.isEmpty()) {
        setErrorMessage(tr("Bad range"));
        setErrorDetails(QString("port='%1' sep='%2' port2='%3'").arg(port, sepStr, port2));
        return ErrorBadRangeFormat;
    }

    return parsePortRange(port, port2, portRangeMap, pairSize);
}

PortRange::ParseError PortRange::parsePortRange(const QStringView &port, const QStringView &port2,
        portrange_map_t &portRangeMap, int &pairSize)
{
    quint16 from = 0, to = 0;

    if (!(parsePortNumber(port, from) && parsePortNumber(port2, to)))
        return ErrorBadPort;

    if (to == 0) {
        to = from;
    }

    portRangeMap.insert(from, to);

    if (from != to) {
        ++pairSize;
    }

    return ErrorOk;
}

bool PortRange::parsePortNumber(const QStringView &port, quint16 &v)
{
    if (port.isEmpty())
        return true;

    bool ok;

    if (port.at(0).isDigit()) {
        v = port.toUShort(&ok);
    } else {
        const char *proto = isProtoTcp() ? "tcp" : (isProtoUdp() ? "udp" : nullptr);

        v = NetUtil::serviceToPort(port, proto, ok);
    }

    if (!ok) {
        setErrorMessage(tr("Bad Port"));
        setErrorDetails(QString("Port='%1'").arg(port));
    }
    return ok;
}
