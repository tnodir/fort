#include "statconnbasejob.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statconnmanager.h"

SqliteDb *StatConnBaseJob::sqliteDb() const
{
    return manager()->sqliteDb();
}

bool StatConnBaseJob::mergeJob(const WorkerJob &job)
{
    const auto &statJob = static_cast<const StatConnBaseJob &>(job);

    return jobType() == statJob.jobType() && processMerge(statJob);
}

void StatConnBaseJob::doJob(WorkerManager *manager)
{
    m_manager = static_cast<StatConnManager *>(manager);

    processJob();
}

void StatConnBaseJob::reportResult(WorkerManager * /*manager*/)
{
    if (resultCount() > 0) {
        emitFinished();
    }
}

SqliteStmt *StatConnBaseJob::getStmt(const char *sql)
{
    return sqliteDb()->stmt(sql);
}

SqliteStmt *StatConnBaseJob::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}
