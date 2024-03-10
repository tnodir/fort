#include "ftstablesqlmodel.h"

namespace {

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

void FtsTableSqlModel::setFtsFilter(const QString &filter)
{
    if (m_ftsFilter == filter)
        return;

    m_ftsFilter = filter;

    m_ftsFilterMatch = makeFtsFilterMatch(m_ftsFilter);

    resetLater();
}

void FtsTableSqlModel::fillQueryVars(QVariantHash &vars) const
{
    if (!ftsFilterMatch().isEmpty()) {
        vars.insert(":match", ftsFilterMatch());
    }
}

QString FtsTableSqlModel::sqlWhere() const
{
    if (ftsFilterMatch().isEmpty())
        return {};

    return sqlWhereFts();
}
