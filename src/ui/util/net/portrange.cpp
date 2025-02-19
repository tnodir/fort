#include "portrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>
#include <util/net/netutil.h>
#include <util/stringutil.h>

PortRange::PortRange(QObject *parent) : ValueRange(parent) { }

bool PortRange::isEmpty() const
{
    return portSize() == 0 && pairSize() == 0;
}

bool PortRange::checkSize() const
{
    return (portSize() + pairSize()) < FORT_CONF_PORT_MAX;
}

int PortRange::sizeToWrite() const
{
    return FORT_CONF_PORT_LIST_SIZE(portSize(), pairSize());
}

void PortRange::clear()
{
    ValueRange::clear();

    m_portArray.clear();
    m_pairFromArray.clear();
    m_pairToArray.clear();
}

void PortRange::toList(QStringList &list) const
{
    for (int i = 0, n = portSize(); i < n; ++i) {
        const port_t port = portAt(i);

        list << QString::number(port);
    }

    for (int i = 0, n = pairSize(); i < n; ++i) {
        const PortPair pair = pairAt(i);

        list << QString("%1-%2").arg(QString::number(pair.from), QString::number(pair.to));
    }
}

bool PortRange::fromList(const StringViewList &list, bool /*sort*/)
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

    fillRangeArrays<port_t>({
            .rangeMap = portRangeMap,
            .valuesArray = m_portArray,
            .pairFromArray = m_pairFromArray,
            .pairToArray = m_pairToArray,
            .pairSize = pairSize,
    });

    return true;
}

PortRange::ParseError PortRange::parsePortLine(
        const QStringView line, portrange_map_t &portRangeMap, int &pairSize)
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

PortRange::ParseError PortRange::parsePortRange(const QStringView port, const QStringView port2,
        portrange_map_t &portRangeMap, int &pairSize)
{
    port_t from = 0, to = 0;

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

bool PortRange::parsePortNumber(const QStringView port, port_t &v)
{
    if (port.isEmpty())
        return true;

    bool ok;

    if (port.at(0).isDigit()) {
        v = port.toUShort(&ok);
    } else {
        const auto proto = isProtoTcp() ? NetUtil::ProtoTcp
                                        : (isProtoUdp() ? NetUtil::ProtoUdp : NetUtil::ProtoAny);

        v = NetUtil::serviceToPort(port, proto, ok);
    }

    if (!ok) {
        setErrorMessage(tr("Bad Port"));
        setErrorDetails(QString("Port='%1'").arg(port));
    }
    return ok;
}

void PortRange::write(ConfData &confData) const
{
    confData.writePortRange(*this);
}
