#include "tablesqlmodel.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

TableSqlModel::TableSqlModel(QObject *parent) : TableItemModel(parent) { }

int TableSqlModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if (m_rowCount < 0) {
        m_rowCount = doSqlCount();
    }

    return m_rowCount;
}

void TableSqlModel::sort(int column, Qt::SortOrder order)
{
    if (m_sortColumn != column || m_sortOrder != order) {
        m_sortColumn = column;
        m_sortOrder = order;

        reset();
    }
}

void TableSqlModel::invalidateRowCache()
{
    m_rowCount = -1;
    TableItemModel::invalidateRowCache();
    emit modelChanged();
}

void TableSqlModel::fillSqlVars(QVariantList &vars) const
{
    Q_UNUSED(vars);
}

int TableSqlModel::doSqlCount() const
{
    QVariantList vars;
    fillSqlVars(vars);

    const auto sqlUtf8 = sqlCount().toUtf8();

    return sqliteDb()->executeEx(sqlUtf8, vars).toInt();
}

QString TableSqlModel::sqlCount() const
{
    return "SELECT COUNT(*) FROM (" + sqlBase() + sqlWhere() + ");";
}

QString TableSqlModel::sql() const
{
    return sqlBase() + sqlWhere() + sqlOrder() + sqlLimitOffset() + ';';
}

QString TableSqlModel::sqlWhere() const
{
    return QString();
}

QString TableSqlModel::sqlOrder() const
{
    if (sortColumn() == -1)
        return QString();

    return " ORDER BY " + sqlOrderColumn();
}

QString TableSqlModel::sqlOrderAsc() const
{
    return (sortOrder() == Qt::AscendingOrder) ? " ASC" : " DESC";
}

QString TableSqlModel::sqlOrderColumn() const
{
    return QString::number(sortColumn()) + sqlOrderAsc();
}

QString TableSqlModel::sqlLimitOffset() const
{
    return " LIMIT 1 OFFSET :row";
}
