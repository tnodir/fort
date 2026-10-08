#ifndef FILTERLINE_H
#define FILTERLINE_H

#include <QStringList>
#include <QVector>

#include "filterlinetext.h"
#include "ruletextparser.h"

// A rule text line of filters by their types (FORT_RULE_FILTER_TYPE_*), parsed by RuleTextParser
class FilterLine
{
public:
    explicit FilterLine(const QString &text = {});
    explicit FilterLine(const FilterLineText &text);

    bool parse();

    const RuleFilter *filter(qint8 type) const; // nullptr - no filter

    static QStringList values(const RuleFilter *filter);

private:
    FilterLineText m_text;

    RuleTextParser m_parser;

    QVector<int> m_filterIndexes; // by the type, -1 - no filter
};

#endif // FILTERLINE_H
