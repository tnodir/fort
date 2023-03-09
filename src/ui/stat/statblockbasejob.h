#ifndef STATBLOCKBASEJOB_H
#define STATBLOCKBASEJOB_H

#include <sqlite/sqlitetypes.h>

#include <util/worker/workerjob.h>

class StatBlockManager;

class StatBlockBaseJob : public WorkerJob
{
public:
    enum StatBlockJobType : qint8 { JobTypeBlockedIp, JobTypeDeleteConn };

    StatBlockManager *manager() const { return m_manager; }
    SqliteDb *sqliteDb() const;

    bool mergeJob(const WorkerJob &job) override;

    void doJob(WorkerObject &worker) override;
    void reportResult(WorkerObject &worker) override;

    virtual StatBlockJobType jobType() const = 0;

protected:
    virtual bool processMerge(const StatBlockBaseJob &statJob) = 0;
    virtual void processJob() = 0;
    virtual void emitFinished() = 0;

    int resultCount() const { return m_resultCount; }
    void setResultCount(int v) { m_resultCount = v; }

    SqliteStmt *getStmt(const char *sql);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);

private:
    int m_resultCount = 0;

    StatBlockManager *m_manager = nullptr;
};

#endif // STATBLOCKBASEJOB_H
