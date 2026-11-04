#include "shiftconnidsjob.h"

#include <algorithm>

#include <sqlite/dbutil.h>
#include <sqlite/sqlitestmt.h>

#include "statconnmanager.h"
#include "statsql.h"

ShiftConnIdsJob::ShiftConnIdsJob(const QVector<qint64> &connIds) : DeleteConnIdsJob(connIds) { }

void ShiftConnIdsJob::deleteConns()
{
    DeleteConnIdsJob::deleteConns();

    shiftConnIds();
}

void ShiftConnIdsJob::emitFinished()
{
    emit manager()->connIdsShifted();
}

void ShiftConnIdsJob::shiftConnIds()
{
    const qint64 connIdFrom = *std::min_element(connIds().cbegin(), connIds().cend());

    /* In ascending order each new id is already free: deleted or shifted */
    qint64 newConnId = connIdFrom;

    for (const qint64 connId : selectConnIdsAfter(connIdFrom)) {
        updateConnId(connId, newConnId++);
    }
}

QVector<qint64> ShiftConnIdsJob::selectConnIdsAfter(qint64 connId)
{
    QVector<qint64> connIds;

    SqliteStmt *stmt = getIdStmt(StatSql::sqlSelectConnIdsAfter, connId);

    while (stmt->step() == SqliteStmt::StepRow) {
        connIds.append(stmt->columnInt64(0));
    }

    stmt->reset();

    return connIds;
}

void ShiftConnIdsJob::updateConnId(qint64 connId, qint64 newConnId)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlUpdateConnId);

    stmt->bindInt64(1, newConnId);
    stmt->bindInt64(2, connId);

    DbUtil::doList({ stmt });
}
