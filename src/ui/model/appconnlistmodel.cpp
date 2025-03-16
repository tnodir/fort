#include "appconnlistmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

namespace {

const char *const sqlSelectAppConnIds = "SELECT conn_id"
                                        "  FROM conn"
                                        "  WHERE app_id = ("
                                        "    SELECT app_id FROM app WHERE path = ?1"
                                        "  )"
                                        "  ORDER BY conn_id DESC"
                                        "  LIMIT 100;";

}

AppConnListModel::AppConnListModel(QObject *parent) : ConnListModel(parent) { }

void AppConnListModel::fillConnIdRange(qint64 &idMin, qint64 &idMax)
{
    m_connIds.clear();

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAppConnIds).vars({ appPath() }).prepare(stmt))
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
