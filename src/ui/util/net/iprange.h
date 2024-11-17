#ifndef IPRANGE_H
#define IPRANGE_H

#include <common/common_types.h>

#include "valuerange.h"

using ip4_t = quint32;

using ip4range_map_t = QMap<ip4_t, ip4_t>;
using ip4_arr_t = QVector<ip4_t>;

using Ip4Pair = ValuePair<ip4_t>;
using Ip6Pair = ValuePair<ip6_addr_t>;

using ip6_pair_arr_t = QVector<Ip6Pair>;
using ip6_arr_t = QVector<ip6_addr_t>;

class IpRange : public ValueRange
{
    Q_OBJECT

public:
    explicit IpRange(QObject *parent = nullptr);

    qint8 emptyNetMask() const { return m_emptyNetMask; }
    void setEmptyNetMask(qint8 v) { m_emptyNetMask = v; }

    const ip4_arr_t &ip4Array() const { return m_ip4Array; }
    ip4_arr_t &ip4Array() { return m_ip4Array; }

    const ip4_arr_t &pair4FromArray() const { return m_pair4FromArray; }
    ip4_arr_t &pair4FromArray() { return m_pair4FromArray; }

    const ip4_arr_t &pair4ToArray() const { return m_pair4ToArray; }
    ip4_arr_t &pair4ToArray() { return m_pair4ToArray; }

    int ip4Size() const { return m_ip4Array.size(); }
    int pair4Size() const { return m_pair4ToArray.size(); }

    ip4_t ip4At(int i) const { return m_ip4Array.at(i); }
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

    bool isEmpty() const override;

    void clear() override;

    QString toText() const override;

    bool fromList(const StringViewList &list, bool sort = true) override;

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

    IpRange::ParseError parseIpLine(
            const QStringView &line, ip4range_map_t &ip4RangeMap, int &pair4Size);

    IpRange::ParseError parseIp4Address(const QStringView &ip, const QStringView &mask,
            ip4range_map_t &ip4RangeMap, int &pair4Size, char maskSep);

    IpRange::ParseError parseIp4AddressMask(
            const QStringView &mask, ip4_t &from, ip4_t &to, char maskSep);
    IpRange::ParseError parseIp4AddressMaskFull(const QStringView &mask, ip4_t &from, ip4_t &to);
    IpRange::ParseError parseIp4AddressMaskPrefix(const QStringView &mask, ip4_t &from, ip4_t &to);

    IpRange::ParseError parseIp6Address(
            const QStringView &ip, const QStringView &mask, char maskSep);

    IpRange::ParseError parseIp6AddressMask(
            const QStringView &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask, char maskSep);
    IpRange::ParseError parseIp6AddressMaskFull(
            const QStringView &mask, ip6_addr_t &to, bool &hasMask);
    IpRange::ParseError parseIp6AddressMaskPrefix(
            const QStringView &mask, ip6_addr_t &from, ip6_addr_t &to, bool &hasMask);

private:
    qint8 m_emptyNetMask = 32;

    ip4_arr_t m_ip4Array;
    ip4_arr_t m_pair4FromArray;
    ip4_arr_t m_pair4ToArray;

    ip6_arr_t m_ip6Array;
    ip6_arr_t m_pair6FromArray;
    ip6_arr_t m_pair6ToArray;
};

#endif // IPRANGE_H
