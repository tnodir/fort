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

void TableSqlModel::fillSqlVars(QVariantMap &varsMap) const
{
    Q_UNUSED(varsMap);
}

int TableSqlModel::doSqlCount() const
{
    QVariantMap varsMap;
    fillSqlVars(varsMap);

    const auto sqlUtf8 = sqlCount().toUtf8();

    return sqliteDb()->executeEx(sqlUtf8, {}, varsMap).toInt();
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

    return QString(" ORDER BY %1 %2").arg(sqlOrderColumn(), sqlOrderAsc());
}

QString TableSqlModel::sqlOrderAsc() const
{
    return (sortOrder() == Qt::AscendingOrder) ? "ASC" : "DESC";
}

QString TableSqlModel::sqlOrderColumn() const
{
    return QString::number(sortColumn());
}

QString TableSqlModel::sqlLimitOffset() const
{
    return " LIMIT 1 OFFSET :row";
}
