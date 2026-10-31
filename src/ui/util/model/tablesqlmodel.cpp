#include "tablesqlmodel.h"

#include <QSet>

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

QVector<qint64> TableSqlModel::idsByRows(const QVector<int> &rows) const
{
    QVector<qint64> idList;
    if (rows.isEmpty())
        return idList;

    walkIds([&](int row, qint64 id) {
        if (row == rows.at(idList.size())) {
            idList.append(id);
        }
        return idList.size() < rows.size();
    });

    return idList;
}

QVector<TableSqlModel::IdRow> TableSqlModel::rowsByIds(const QVector<qint64> &idList) const
{
    const QSet<qint64> idSet(idList.cbegin(), idList.cend());

    QVector<IdRow> rows;

    walkIds([&](int row, qint64 id) {
        if (idSet.contains(id)) {
            rows.append({ id, row });
        }
        return rows.size() < idList.size();
    });

    return rows;
}

void TableSqlModel::walkIds(const std::function<bool(int row, qint64 id)> &visit) const
{
    QVariantHash vars;
    fillQueryVars(vars);

    const auto sql = sqlBase() + sqlWhere() + sqlOrder() + sqlEnd() + ';';

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepare(stmt))
        return;

    for (int row = 0; stmt.step() == SqliteStmt::StepRow; ++row) {
        if (!visit(row, stmt.columnInt64(0)))
            break;
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
    return "SELECT COUNT(*) FROM (" + sqlBase() + sqlWhere() + sqlEnd() + ");";
}

QString TableSqlModel::sql() const
{
    return sqlBase() + sqlWhere() + sqlOrder() + sqlEnd() + sqlLimitOffset() + ';';
}

QString TableSqlModel::sqlOrder() const
{
    if (sortColumn() == -1)
        return QString();

    return " ORDER BY " + sqlOrderColumn();
}

QString TableSqlModel::sqlOrderAsc() const
{
    return isAscendingOrder() ? " ASC" : " DESC";
}

QString TableSqlModel::sqlOrderColumn() const
{
    return QString::number(sortColumn()) + sqlOrderAsc();
}

QString TableSqlModel::sqlLimitOffset() const
{
    return " LIMIT 1 OFFSET :offset";
}
