#ifndef PORTRANGE_H
#define PORTRANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include <common/common_types.h>
#include <util/util_types.h>

using PortPair = struct
{
    quint16 from, to;
};

using portrange_map_t = QMap<quint16, quint16>;
using port_arr_t = QVector<quint16>;

class PortRange : public QObject
{
    Q_OBJECT

public:
    explicit PortRange(QObject *parent = nullptr);

    bool isProtoTcp() const { return m_isProtoTcp; }
    void setProtoTcp(bool v) { m_isProtoTcp = v; }

    bool isProtoUdp() const { return m_isProtoUdp; }
    void setProtoUdp(bool v) { m_isProtoUdp = v; }

    bool isProtoAny() const { return isProtoTcp() == isProtoUdp(); }

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorDetails() const { return m_errorDetails; }
    QString errorLineAndMessageDetails() const;

    const port_arr_t &portArray() const { return m_portArray; }
    port_arr_t &portArray() { return m_portArray; }

    const port_arr_t &pairFromArray() const { return m_pairFromArray; }
    port_arr_t &pairFromArray() { return m_pairFromArray; }

    const port_arr_t &pair4oArray() const { return m_pairToArray; }
    port_arr_t &pairToArray() { return m_pairToArray; }

    int portSize() const { return m_portArray.size(); }
    int pairSize() const { return m_pairToArray.size(); }

    quint16 portAt(int i) const { return m_portArray.at(i); }
    PortPair pairAt(int i) const { return PortPair { m_pairFromArray.at(i), m_pairToArray.at(i) }; }

    bool isEmpty() const;

    QString toText() const;

    // Parse Port ranges
    bool fromText(const QString &text);
    bool fromList(const StringViewList &list);

public slots:
    void clear();

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadFormat,
        ErrorBadRangeFormat,
        ErrorBadPort,
    };

    void setErrorLineNo(int lineNo) { m_errorLineNo = lineNo; }
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }
    void setErrorDetails(const QString &errorDetails) { m_errorDetails = errorDetails; }

    void appendErrorDetails(const QString &errorDetails);

    PortRange::ParseError parsePortLine(
            const QStringView &line, portrange_map_t &portRangeMap, int &pairSize);

    PortRange::ParseError parsePortRange(const QStringView &port, const QStringView &port2,
            portrange_map_t &portRangeMap, int &pairSize);

    bool parsePortNumber(const QStringView &port, quint16 &v);

    void fillPortRange(const portrange_map_t &portRangeMap, int pairSize);

private:
    bool m_isProtoTcp : 1 = false;
    bool m_isProtoUdp : 1 = false;

    int m_errorLineNo = 0;
    QString m_errorMessage;
    QString m_errorDetails;

    port_arr_t m_portArray;
    port_arr_t m_pairFromArray;
    port_arr_t m_pairToArray;
};

#endif // PORTRANGE_H
