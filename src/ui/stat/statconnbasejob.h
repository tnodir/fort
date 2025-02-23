#ifndef STATCONNBASEJOB_H
#define STATCONNBASEJOB_H

#include <sqlite/sqliteutilbase.h>

#include <util/worker/workerjob.h>

class StatConnManager;

class StatConnBaseJob : public WorkerJob, public SqliteUtilBase
{
public:
    enum StatConnJobType : qint8 { JobTypeLogConn, JobTypeDeleteConn };

    StatConnManager *manager() const { return m_manager; }
    SqliteDb *sqliteDb() const override;

    bool mergeJob(const WorkerJob &job) override;

    void doJob(WorkerObject &worker) override;
    void reportResult(WorkerObject &worker) override;

    virtual StatConnJobType jobType() const = 0;

protected:
    virtual bool processMerge(const StatConnBaseJob &statJob) = 0;
    virtual void processJob() = 0;
    virtual void emitFinished() = 0;

    int resultCount() const { return m_resultCount; }
    void setResultCount(int v) { m_resultCount = v; }

    SqliteStmt *getStmt(const char *sql);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);

private:
    int m_resultCount = 0;

    StatConnManager *m_manager = nullptr;
};

#endif // STATCONNBASEJOB_H
