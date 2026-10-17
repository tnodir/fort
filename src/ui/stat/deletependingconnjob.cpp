#include "deletependingconnjob.h"

#include <sqlite/dbutil.h>

#include "statconnmanager.h"
#include "statsql.h"

DeletePendingConnJob::DeletePendingConnJob(const QVector<qint64> &connIds) : m_connIds(connIds) { }

bool DeletePendingConnJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const DeletePendingConnJob &>(statJob);

    m_connIds.append(job.connIds());

    return true;
}

void DeletePendingConnJob::processJob()
{
    beginWriteTransaction();

    for (const qint64 connId : std::as_const(m_connIds)) {
        DbUtil::doList({ getIdStmt(StatSql::sqlDeleteConnById, connId) });
    }

    DbUtil::doList({ getStmt(StatSql::sqlDeleteConnApps) });

    commitTransaction();

    setResultCount(m_connIds.size());
}

void DeletePendingConnJob::emitFinished()
{
    emit manager()->deleteConnFinished(m_connIds.constLast());
}
