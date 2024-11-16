#ifndef PROTORANGE_H
#define PROTORANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include <common/common_types.h>
#include <util/util_types.h>

using ProtoPair = struct
{
    quint8 from, to;
};

using protorange_map_t = QMap<quint8, quint8>;
using proto_arr_t = QVector<quint8>;

class ProtoRange : public QObject
{
    Q_OBJECT

public:
    explicit ProtoRange(QObject *parent = nullptr);

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorDetails() const { return m_errorDetails; }
    QString errorLineAndMessageDetails() const;

    const proto_arr_t &protoArray() const { return m_protoArray; }
    proto_arr_t &protoArray() { return m_protoArray; }

    const proto_arr_t &pairFromArray() const { return m_pairFromArray; }
    proto_arr_t &pairFromArray() { return m_pairFromArray; }

    const proto_arr_t &pair4oArray() const { return m_pairToArray; }
    proto_arr_t &pairToArray() { return m_pairToArray; }

    int protoSize() const { return m_protoArray.size(); }
    int pairSize() const { return m_pairToArray.size(); }

    quint8 protoAt(int i) const { return m_protoArray.at(i); }
    ProtoPair pairAt(int i) const
    {
        return ProtoPair { m_pairFromArray.at(i), m_pairToArray.at(i) };
    }

    bool isEmpty() const;

    QString toText() const;

    // Parse Protocol ranges
    bool fromText(const QString &text);
    bool fromList(const StringViewList &list);

public slots:
    void clear();

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadFormat,
        ErrorBadRangeFormat,
        ErrorBadProto,
    };

    void setErrorLineNo(int lineNo) { m_errorLineNo = lineNo; }
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }
    void setErrorDetails(const QString &errorDetails) { m_errorDetails = errorDetails; }

    void appendErrorDetails(const QString &errorDetails);

    ProtoRange::ParseError parseProtoLine(
            const QStringView &line, protorange_map_t &protoRangeMap, int &pairSize);

    ProtoRange::ParseError parseProtoRange(const QStringView &proto, const QStringView &proto2,
            protorange_map_t &protoRangeMap, int &pairSize);

    bool parseProtoNumber(const QStringView &proto, quint8 &v);

    void fillProtoRange(const protorange_map_t &protoRangeMap, int pairSize);

private:
    int m_errorLineNo = 0;
    QString m_errorMessage;
    QString m_errorDetails;

    proto_arr_t m_protoArray;
    proto_arr_t m_pairFromArray;
    proto_arr_t m_pairToArray;
};

#endif // PROTORANGE_H
