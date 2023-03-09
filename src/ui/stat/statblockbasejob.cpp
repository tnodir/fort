#include "statblockbasejob.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"

SqliteDb *StatBlockBaseJob::sqliteDb() const
{
    return manager()->sqliteDb();
}

bool StatBlockBaseJob::mergeJob(const WorkerJob &job)
{
    const auto &statJob = static_cast<const StatBlockBaseJob &>(job);

    return jobType() == statJob.jobType() && processMerge(statJob);
}

void StatBlockBaseJob::doJob(WorkerObject &worker)
{
    m_manager = static_cast<StatBlockManager *>(worker.manager());

    processJob();
}

void StatBlockBaseJob::reportResult(WorkerObject & /*worker*/)
{
    if (resultCount() > 0) {
        emitFinished();
    }
}

SqliteStmt *StatBlockBaseJob::getStmt(const char *sql)
{
    return sqliteDb()->stmt(sql);
}

SqliteStmt *StatBlockBaseJob::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}
