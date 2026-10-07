#include "filterline.h"

#include <common/fortconf.h>

namespace {

// By FORT_RULE_FILTER_TYPE_*
const char *const filterNames[] = { "IP", "Port", "Local_IP", "Local_Port", "Proto", "IP_Ver",
    "Dir", "Zones", "Area", "Profile", "Act", "Opt" };

inline constexpr int filterTypesCount = std::size(filterNames);

static_assert(filterTypesCount == FORT_RULE_FILTER_TYPE_PORT_TCP, "Filter names mismatch");

}

FilterLine::FilterLine(const QString &text) :
    m_text(text), m_filterIndexes(filterTypesCount, -1) { }

void FilterLine::addFilter(qint8 type, const QString &values, bool isNot)
{
    if (values.isEmpty())
        return;

    if (!m_text.isEmpty()) {
        m_text += ':';
    }

    // e.g. "IP(1.1.1.1)", "!Port(80, 443)"
    if (isNot) {
        m_text += '!';
    }

    m_text += QLatin1String(filterNames[type]) + '(' + values + ')';
}

bool FilterLine::parse()
{
    m_filterIndexes.fill(-1);

    m_parser.setText(m_text);
    if (!m_parser.parse())
        return false;

    const auto &ruleFilters = m_parser.ruleFilters();

    for (int i = 0, n = ruleFilters.size(); i < n; ++i) {
        const qint8 type = ruleFilters[i].type;

        // Skip the lists and e.g. "tcp(80)"
        if (type >= 0 && type < filterTypesCount) {
            m_filterIndexes[type] = i;
        }
    }

    return true;
}

const RuleFilter *FilterLine::filter(qint8 type) const
{
    const int index = m_filterIndexes.value(type, -1);

    return (index >= 0) ? &m_parser.ruleFilters()[index] : nullptr;
}
