#include "appconnlistmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <stat/statsql.h>

namespace {

inline constexpr int appConnsMax = 100;

}

AppConnListModel::AppConnListModel(QObject *parent) : ConnListModel(parent) { }

void AppConnListModel::fillConnIdRange(qint64 &idMin, qint64 &idMax)
{
    m_connIds.clear();

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb())
                    .sql(StatSql::sqlSelectAppConnIds)
                    .vars({
                            DbVar::nullable(confAppId()),
                            DbVar::nullable(appPath()),
                            appConnsMax,
                            0,
                    })
                    .prepare(stmt))
        return;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 appId = stmt.columnInt64(0);
        m_connIds.append(appId);
    }

    if (!m_connIds.isEmpty()) {
        idMin = m_connIds.last();
        idMax = m_connIds.first();
    }
}

bool AppConnListModel::isConnIdRangeOut(
        qint64 /*oldIdMin*/, qint64 /*oldIdMax*/, qint64 /*idMin*/, qint64 /*idMax*/) const
{
    return false; // always reset on any changes
}

qint64 AppConnListModel::connIdByIndex(int row) const
{
    if (isAscendingOrder()) {
        row = m_connIds.size() - row - 1;
    }

    return m_connIds.value(row);
}
