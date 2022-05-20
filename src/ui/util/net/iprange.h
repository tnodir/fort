#ifndef IPRANGE_H
#define IPRANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include <common/common_types.h>
#include <fortcompat.h>

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

using ip6_arr_t = QVector<ip6_addr_t>;

class IpRange : public QObject
{
    Q_OBJECT

public:
    explicit IpRange(QObject *parent = nullptr);

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorLineAndMessage() const;

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

    // Parse IPv4 ranges
    bool fromText(const QString &text);
    bool fromList(const StringViewList &list, int emptyNetMask = 32, bool sort = true);

signals:
    void errorLineNoChanged();
    void errorMessageChanged();

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

    void setErrorLineNo(int lineNo);
    void setErrorMessage(const QString &errorMessage);

    IpRange::ParseError parseIpLine(
            const StringView line, ip4range_map_t &ip4RangeMap, int &pair4Size, int emptyNetMask);

    IpRange::ParseError parseIp4Address(const QString &ip, const QString &mask,
            ip4range_map_t &ip4RangeMap, int &pair4Size, int emptyNetMask, char maskSep);

    IpRange::ParseError parseIp4AddressMask(
            const QString &mask, quint32 &from, quint32 &to, int emptyNetMask, char maskSep);
    IpRange::ParseError parseIp4AddressMaskFull(const QString &mask, quint32 &from, quint32 &to);
    IpRange::ParseError parseIp4AddressMaskPrefix(
            const QString &mask, quint32 &from, quint32 &to, int emptyNetMask);

    IpRange::ParseError parseIp6Address(const QString &ip, const QString &mask, char maskSep);

    IpRange::ParseError parseIp6AddressMask(
            const QString &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask, char maskSep);
    IpRange::ParseError parseIp6AddressMaskFull(const QString &mask, ip6_addr_t &to, bool &hasMask);
    IpRange::ParseError parseIp6AddressMaskPrefix(
            const QString &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask);

    void fillIp4Range(const ip4range_map_t &ipRangeMap, int pairSize);

private:
    int m_errorLineNo = 0;
    QString m_errorMessage;

    ip4_arr_t m_ip4Array;
    ip4_arr_t m_pair4FromArray;
    ip4_arr_t m_pair4ToArray;

    ip6_arr_t m_ip6Array;
    ip6_arr_t m_pair6FromArray;
    ip6_arr_t m_pair6ToArray;
};

#endif // IPRANGE_H
