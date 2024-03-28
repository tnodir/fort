#include "tablesqlmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

TableSqlModel::TableSqlModel(QObject *parent) : TableItemModel(parent) { }

int TableSqlModel::rowCount(const QModelIndex & /*parent*/) const
{
    if (m_sqlRowCount < 0) {
        m_sqlRowCount = doSqlCount();
    }

    return m_sqlRowCount;
}

void TableSqlModel::sort(int column, Qt::SortOrder order)
{
    if (m_sortColumn != column || m_sortOrder != order) {
        m_sortColumn = column;
        m_sortOrder = order;

        reset();
    }
}

void TableSqlModel::invalidateRowCache() const
{
    setSqlRowCount(-1);
    TableItemModel::invalidateRowCache();
}

void TableSqlModel::fillQueryVarsForRow(QVariantHash &vars, int row) const
{
    fillQueryVars(vars);
    vars.insert(":offset", row);
}

int TableSqlModel::doSqlCount() const
{
    QVariantHash vars;
    fillQueryVars(vars);

    return DbQuery(sqliteDb()).sql(sqlCount()).vars(vars).execute().toInt();
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
    return " LIMIT 1 OFFSET :offset";
}
