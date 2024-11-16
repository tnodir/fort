#include "protorange.h"

#include <util/net/netutil.h>
#include <util/stringutil.h>

ProtoRange::ProtoRange(QObject *parent) : QObject(parent) { }

void ProtoRange::clear()
{
    m_errorLineNo = 0;
    m_errorMessage.clear();
    m_errorDetails.clear();

    m_protoArray.clear();
    m_pairFromArray.clear();
    m_pairToArray.clear();
}

void ProtoRange::appendErrorDetails(const QString &errorDetails)
{
    m_errorDetails += (m_errorDetails.isEmpty() ? QString() : QString(' ')) + errorDetails;
}

QString ProtoRange::errorLineAndMessageDetails() const
{
    return tr("Error at line %1: %2 (%3)")
            .arg(QString::number(errorLineNo()), errorMessage(), errorDetails());
}

bool ProtoRange::isEmpty() const
{
    return protoSize() == 0 && pairSize() == 0;
}

QString ProtoRange::toText() const
{
    QString text;

    for (int i = 0, n = protoSize(); i < n; ++i) {
        const quint8 proto = protoAt(i);

        text += QString("%1\n").arg(NetUtil::protocolName(proto));
    }

    for (int i = 0, n = pairSize(); i < n; ++i) {
        const ProtoPair pair = pairAt(i);

        text += QString("%1-%2\n").arg(QString::number(pair.from), QString::number(pair.to));
    }

    return text;
}

bool ProtoRange::fromText(const QString &text)
{
    const auto list = StringUtil::splitView(text, QLatin1Char('\n'));
    return fromList(list);
}

bool ProtoRange::fromList(const StringViewList &list)
{
    clear();

    protorange_map_t protoRangeMap;
    int pairSize = 0;

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (parseProtoLine(line, protoRangeMap, pairSize) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    fillProtoRange(protoRangeMap, pairSize);

    return true;
}

ProtoRange::ParseError ProtoRange::parseProtoLine(
        const QStringView &line, protorange_map_t &protoRangeMap, int &pairSize)
{
    static const QRegularExpression protoRe(R"(^([\d\w]+)\s*(-?)\s*(\S*))");

    const auto match = StringUtil::match(protoRe, line);
    if (!match.hasMatch()) {
        setErrorMessage(tr("Bad format"));
        return ErrorBadFormat;
    }

    const auto proto = match.captured(1);
    const auto sepStr = match.capturedView(2);
    const auto proto2 = match.captured(3);

    if (sepStr.isEmpty() != proto2.isEmpty()) {
        setErrorMessage(tr("Bad range"));
        setErrorDetails(QString("proto='%1' sep='%2' proto2='%3'").arg(proto, sepStr, proto2));
        return ErrorBadRangeFormat;
    }

    return parseProtoRange(proto, proto2, protoRangeMap, pairSize);
}

ProtoRange::ParseError ProtoRange::parseProtoRange(const QStringView &proto,
        const QStringView &proto2, protorange_map_t &protoRangeMap, int &pairSize)
{
    quint8 from = 0, to = 0;

    if (!(parseProtoNumber(proto, from) && parseProtoNumber(proto2, to)))
        return ErrorBadProto;

    if (to == 0) {
        to = from;
    }

    protoRangeMap.insert(from, to);

    if (from != to) {
        ++pairSize;
    }

    return ErrorOk;
}

bool ProtoRange::parseProtoNumber(const QStringView &proto, quint8 &v)
{
    if (proto.isEmpty())
        return true;

    bool ok;

    if (proto.at(0).isDigit()) {
        v = proto.toUShort(&ok);
    } else {
        v = NetUtil::protocolNumber(proto);
        ok = (v != 0);
    }

    if (!ok) {
        setErrorMessage(tr("Bad Protocol"));
        setErrorDetails(QString("Protocol='%1'").arg(proto));
    }
    return ok;
}

void ProtoRange::fillProtoRange(const protorange_map_t &protoRangeMap, int pairSize)
{
    if (protoRangeMap.isEmpty())
        return;

    const int mapSize = protoRangeMap.size();
    m_protoArray.reserve(mapSize - pairSize);
    m_pairFromArray.reserve(pairSize);
    m_pairToArray.reserve(pairSize);

    ProtoPair prevProto;
    int prevIndex = -1;

    auto it = protoRangeMap.constBegin();
    auto end = protoRangeMap.constEnd();

    for (; it != end; ++it) {
        ProtoPair proto { it.key(), it.value() };

        // try to merge colliding addresses
        if (prevIndex >= 0 && proto.from <= prevProto.to + 1) {
            if (proto.to > prevProto.to) {
                m_pairToArray.replace(prevIndex, proto.to);

                prevProto.to = proto.to;
            }
            // else skip it
        } else if (proto.from == proto.to) {
            m_protoArray.append(proto.from);
        } else {
            m_pairFromArray.append(proto.from);
            m_pairToArray.append(proto.to);

            prevProto = proto;
            ++prevIndex;
        }
    }
}
