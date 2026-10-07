#ifndef FILTERLINE_H
#define FILTERLINE_H

#include <QString>
#include <QVector>

#include "ruletextparser.h"

// A rule text line of filters by their types (FORT_RULE_FILTER_TYPE_*), parsed by RuleTextParser
class FilterLine
{
public:
    explicit FilterLine(const QString &text = {});

    const QString &text() const { return m_text; }

    // Adds the filter's section with its name to the text: the values are e.g. "80, 443"
    void addFilter(qint8 type, const QString &values, bool isNot = false);

    bool parse();

    const RuleFilter *filter(qint8 type) const; // nullptr - no filter

private:
    QString m_text;

    RuleTextParser m_parser;

    QVector<int> m_filterIndexes; // by the type, -1 - no filter
};

#endif // FILTERLINE_H
