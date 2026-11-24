#include "deletetimedoutconnsjob.h"

#include <sqlite/dbutil.h>
#include <sqlite/sqlitestmt.h>

#include "pendingconnmanager.h"
#include "statsql.h"

DeleteTimedOutConnsJob::DeleteTimedOutConnsJob(qint64 expireTime) : m_expireTime(expireTime) { }

bool DeleteTimedOutConnsJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const DeleteTimedOutConnsJob &>(statJob);

    m_expireTime = job.expireTime();

    return true;
}

void DeleteTimedOutConnsJob::processJob()
{
    beginWriteTransaction();

    deleteConns();

    commitTransaction();

    setResultCount(m_flowIds.size());
}

void DeleteTimedOutConnsJob::emitFinished()
{
    auto manager = static_cast<PendingConnManager *>(this->manager());

    emit manager->timedOutConnsDeleted(m_flowIds);
}

void DeleteTimedOutConnsJob::deleteConns()
{
    m_flowIds = DbUtil::stepIds<quint64>(getIdStmt(StatSql::sqlDeleteTimedOutConns, expireTime()));

    if (!m_flowIds.isEmpty()) {
        DbUtil::doList({ getStmt(StatSql::sqlDeleteConnApps) });
    }
}
