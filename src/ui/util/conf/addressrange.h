#ifndef ADDRESSRANGE_H
#define ADDRESSRANGE_H

#include <QObject>
#include <QVariant>

#include "../net/ip4range.h"

class AddressRange
{
public:
    explicit AddressRange();

    bool includeAll() const { return m_includeAll; }
    void setIncludeAll(bool includeAll) { m_includeAll = includeAll; }

    bool excludeAll() const { return m_excludeAll; }
    void setExcludeAll(bool excludeAll) { m_excludeAll = excludeAll; }

    Ip4Range &includeRange() { return m_includeRange; }
    Ip4Range &excludeRange() { return m_excludeRange; }

    const Ip4Range &includeRange() const { return m_includeRange; }
    const Ip4Range &excludeRange() const { return m_excludeRange; }

private:
    bool m_includeAll   : 1;
    bool m_excludeAll   : 1;

    Ip4Range m_includeRange;
    Ip4Range m_excludeRange;
};

#endif // ADDRESSRANGE_H
