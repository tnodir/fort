#include "deleteconnidsjob.h"

#include <sqlite/dbutil.h>

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

    for (const qint64 connId : std::as_const(m_connIds)) {
        DbUtil::doList({ getIdStmt(StatSql::sqlDeleteConnById, connId) });
    }

    DbUtil::doList({ getStmt(StatSql::sqlDeleteConnApps) });

    commitTransaction();

    setResultCount(m_connIds.size());
}

void DeleteConnIdsJob::emitFinished()
{
    emit manager()->deleteConnFinished(m_connIds.constLast());
}
