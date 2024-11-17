#ifndef PROTORANGE_H
#define PROTORANGE_H

#include <common/common_types.h>

#include "valuerange.h"

using proto_t = quint8;

using protorange_map_t = QMap<proto_t, proto_t>;
using proto_arr_t = QVector<proto_t>;

using ProtoPair = ValuePair<proto_t>;

class ProtoRange : public ValueRange
{
    Q_OBJECT

public:
    explicit ProtoRange(QObject *parent = nullptr);

    const proto_arr_t &protoArray() const { return m_protoArray; }
    proto_arr_t &protoArray() { return m_protoArray; }

    const proto_arr_t &pairFromArray() const { return m_pairFromArray; }
    proto_arr_t &pairFromArray() { return m_pairFromArray; }

    const proto_arr_t &pair4oArray() const { return m_pairToArray; }
    proto_arr_t &pairToArray() { return m_pairToArray; }

    int protoSize() const { return m_protoArray.size(); }
    int pairSize() const { return m_pairToArray.size(); }

    proto_t protoAt(int i) const { return m_protoArray.at(i); }
    ProtoPair pairAt(int i) const
    {
        return ProtoPair { m_pairFromArray.at(i), m_pairToArray.at(i) };
    }

    bool isEmpty() const override;

    void clear() override;

    QString toText() const override;

    bool fromList(const StringViewList &list, bool sort = true) override;

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadFormat,
        ErrorBadRangeFormat,
        ErrorBadProto,
    };

    ProtoRange::ParseError parseProtoLine(
            const QStringView &line, protorange_map_t &protoRangeMap, int &pairSize);

    ProtoRange::ParseError parseProtoRange(const QStringView &proto, const QStringView &proto2,
            protorange_map_t &protoRangeMap, int &pairSize);

    bool parseProtoNumber(const QStringView &proto, proto_t &v);

private:
    proto_arr_t m_protoArray;
    proto_arr_t m_pairFromArray;
    proto_arr_t m_pairToArray;
};

#endif // PROTORANGE_H
