#include "filterline.h"

#include <common/fortconf.h>

namespace {

// The types of the named filters, by FORT_RULE_FILTER_TYPE_*
inline constexpr int filterTypesCount = FORT_RULE_FILTER_TYPE_PORT_TCP;

}

FilterLine::FilterLine(const QString &text) : FilterLine(FilterLineText(text)) { }

FilterLine::FilterLine(const FilterLineText &text) :
    m_text(text), m_filterIndexes(filterTypesCount, -1)
{
}

bool FilterLine::parse()
{
    m_filterIndexes.fill(-1);

    m_parser.setText(m_text.text());
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

QStringList FilterLine::values(const RuleFilter *filter)
{
    if (!filter)
        return {};

    QStringList list;
    for (const QStringView value : filter->values) {
        list << value.toString();
    }

    return list;
}
