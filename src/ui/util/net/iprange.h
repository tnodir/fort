#ifndef IPRANGE_H
#define IPRANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include <common/common_types.h>
#include <util/util_types.h>

using Ip4Pair = struct
{
    quint32 from, to;
};

using Ip6Pair = struct
{
    ip6_addr_t from, to;
};

using ip4range_map_t = QMap<quint32, quint32>;
using ip4_arr_t = QVector<quint32>;

using ip6_pair_arr_t = QVector<Ip6Pair>;
using ip6_arr_t = QVector<ip6_addr_t>;

class IpRange : public QObject
{
    Q_OBJECT

public:
    explicit IpRange(QObject *parent = nullptr);

    qint8 emptyNetMask() const { return m_emptyNetMask; }
    void setEmptyNetMask(qint8 v) { m_emptyNetMask = v; }

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorDetails() const { return m_errorDetails; }
    QString errorLineAndMessageDetails() const;

    const ip4_arr_t &ip4Array() const { return m_ip4Array; }
    ip4_arr_t &ip4Array() { return m_ip4Array; }

    const ip4_arr_t &pair4FromArray() const { return m_pair4FromArray; }
    ip4_arr_t &pair4FromArray() { return m_pair4FromArray; }

    const ip4_arr_t &pair4ToArray() const { return m_pair4ToArray; }
    ip4_arr_t &pair4ToArray() { return m_pair4ToArray; }

    int ip4Size() const { return m_ip4Array.size(); }
    int pair4Size() const { return m_pair4ToArray.size(); }

    quint32 ip4At(int i) const { return m_ip4Array.at(i); }
    Ip4Pair pair4At(int i) const
    {
        return Ip4Pair { m_pair4FromArray.at(i), m_pair4ToArray.at(i) };
    }

    const ip6_arr_t &ip6Array() const { return m_ip6Array; }
    ip6_arr_t &ip6Array() { return m_ip6Array; }

    const ip6_arr_t &pair6FromArray() const { return m_pair6FromArray; }
    ip6_arr_t &pair6FromArray() { return m_pair6FromArray; }

    const ip6_arr_t &pair6ToArray() const { return m_pair6ToArray; }
    ip6_arr_t &pair6ToArray() { return m_pair6ToArray; }

    int ip6Size() const { return m_ip6Array.size(); }
    int pair6Size() const { return m_pair6ToArray.size(); }

    ip6_addr_t ip6At(int i) const { return m_ip6Array.at(i); }
    Ip6Pair pair6At(int i) const
    {
        return Ip6Pair { m_pair6FromArray.at(i), m_pair6ToArray.at(i) };
    }

    bool isEmpty() const;

    QString toText() const;

    // Parse IP ranges
    bool fromText(const QString &text);
    bool fromList(const StringViewList &list, bool sort = true);

public slots:
    void clear();

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadFormat,
        ErrorBadMaskFormat,
        ErrorBadMask,
        ErrorBadAddress,
        ErrorBadAddress2,
        ErrorBadRange,
    };

    void setErrorLineNo(int lineNo) { m_errorLineNo = lineNo; }
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }
    void setErrorDetails(const QString &errorDetails) { m_errorDetails = errorDetails; }

    void appendErrorDetails(const QString &errorDetails);

    IpRange::ParseError parseIpLine(
            const QStringView &line, ip4range_map_t &ip4RangeMap, int &pair4Size);

    IpRange::ParseError parseIp4Address(const QStringView &ip, const QStringView &mask,
            ip4range_map_t &ip4RangeMap, int &pair4Size, char maskSep);

    IpRange::ParseError parseIp4AddressMask(
            const QStringView &mask, quint32 &from, quint32 &to, char maskSep);
    IpRange::ParseError parseIp4AddressMaskFull(const QStringView &mask, quint32 &from, quint32 &to);
    IpRange::ParseError parseIp4AddressMaskPrefix(
            const QStringView &mask, quint32 &from, quint32 &to);

    IpRange::ParseError parseIp6Address(const QStringView &ip, const QStringView &mask, char maskSep);

    IpRange::ParseError parseIp6AddressMask(
            const QStringView &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask, char maskSep);
    IpRange::ParseError parseIp6AddressMaskFull(
            const QStringView &mask, ip6_addr_t &to, bool &hasMask);
    IpRange::ParseError parseIp6AddressMaskPrefix(
            const QStringView &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask);

    void fillIp4Range(const ip4range_map_t &ipRangeMap, int pairSize);

private:
    qint8 m_emptyNetMask = 32;

    int m_errorLineNo = 0;
    QString m_errorMessage;
    QString m_errorDetails;

    ip4_arr_t m_ip4Array;
    ip4_arr_t m_pair4FromArray;
    ip4_arr_t m_pair4ToArray;

    ip6_arr_t m_ip6Array;
    ip6_arr_t m_pair6FromArray;
    ip6_arr_t m_pair6ToArray;
};

#endif // IPRANGE_H
