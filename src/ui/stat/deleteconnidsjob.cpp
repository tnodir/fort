#include "deleteconnidsjob.h"

#include <algorithm>

#include <sqlite/dbutil.h>
#include <sqlite/sqlitestmt.h>

#include "statconnmanager.h"
#include "statsql.h"

DeleteConnIdsJob::DeleteConnIdsJob(const QVector<qint64> &connIds) : m_connIds(connIds) { }

bool DeleteConnIdsJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const DeleteConnIdsJob &>(statJob);

    m_connIds.append(job.connIds());

    return true;
}

void DeleteConnIdsJob::processJob()
{
    beginWriteTransaction();

    deleteConns();

    if (!m_connIds.isEmpty()) {
        processDeletedConns();
    }

    commitTransaction();

    setResultCount(m_connIds.size());
}

void DeleteConnIdsJob::emitFinished()
{
    if (manager()->isConnIdsContiguous()) {
        emit manager()->shiftConnIdsFinished(connIdFrom());
    } else {
        emit manager()->deleteConnFinished(m_connIds.constLast());
    }
}

void DeleteConnIdsJob::deleteConns()
{
    for (const qint64 connId : std::as_const(m_connIds)) {
        DbUtil::doList({ getIdStmt(StatSql::sqlDeleteConnById, connId) });
    }
}

void DeleteConnIdsJob::processDeletedConns()
{
    if (manager()->isConnIdsContiguous()) {
        shiftConnIds();
    }

    DbUtil::doList({ getStmt(StatSql::sqlDeleteConnApps) });
}

qint64 DeleteConnIdsJob::connIdFrom() const
{
    return *std::min_element(m_connIds.cbegin(), m_connIds.cend());
}

void DeleteConnIdsJob::shiftConnIds()
{
    const qint64 connIdFrom = this->connIdFrom();

    /* In ascending order each new id is already free: deleted or shifted */
    qint64 newConnId = connIdFrom;

    const auto connIdsAfter =
            DbUtil::stepIds(getIdStmt(StatSql::sqlSelectConnIdsAfter, connIdFrom));

    for (const qint64 connId : connIdsAfter) {
        updateConnId(connId, newConnId++);
    }
}

void DeleteConnIdsJob::updateConnId(qint64 connId, qint64 newConnId)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlUpdateConnId);

    stmt->bindInt64(1, newConnId);
    stmt->bindInt64(2, connId);

    DbUtil::doList({ stmt });
}
