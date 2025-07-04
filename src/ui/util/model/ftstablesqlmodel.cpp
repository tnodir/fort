#include "ftstablesqlmodel.h"

namespace {

QString makeRegexpFilterColumns(const QStringList &columns)
{
    if (columns.isEmpty())
        return {};

    QStringList list;

    for (const QString &column : columns) {
        list << (column + " REGEXP :regexp");
    }

    return '(' + list.join(" OR ") + ')';
}

QString makeFtsFilterMatch(const QString &filter)
{
    if (filter.isEmpty())
        return {};

    const QStringList words = filter.trimmed().split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty())
        return {};

    return words.join("* ") + '*';
}

}

FtsTableSqlModel::FtsTableSqlModel(QObject *parent) : TableSqlModel(parent) { }

void FtsTableSqlModel::setTextFilter(const QString &filter)
{
    if (m_textFilter == filter)
        return;

    m_textFilter = filter;

    if (filter.startsWith('/')) {
        setupRegexpFilter();
    } else {
        setupFtsFilter();
    }

    resetLater();
}

void FtsTableSqlModel::setupRegexpFilter()
{
    m_regexpFilterMatch = textFilter().mid(1);
    m_regexpFilterColumns = makeRegexpFilterColumns(regexpColumns());

    m_ftsFilterMatch.clear();
}

void FtsTableSqlModel::setupFtsFilter()
{
    m_ftsFilterMatch = makeFtsFilterMatch(textFilter());

    m_regexpFilterMatch.clear();
    m_regexpFilterColumns.clear();
}

void FtsTableSqlModel::fillQueryVars(QVariantHash &vars) const
{
    // Regexp
    if (!regexpFilterMatch().isEmpty()) {
        vars.insert(":regexp", regexpFilterMatch());
    }
    // FTS
    else if (!ftsFilterMatch().isEmpty()) {
        vars.insert(":match", ftsFilterMatch());
    }
}

QString FtsTableSqlModel::sqlWhere() const
{
    if (!regexpFilterMatch().isEmpty()) {
        return sqlWhereRegexp();
    }

    if (!ftsFilterMatch().isEmpty()) {
        return sqlWhereFts();
    }

    return {};
}

QString FtsTableSqlModel::sqlWhereRegexp() const
{
    return " WHERE " + regexpFilterColumns();
}
