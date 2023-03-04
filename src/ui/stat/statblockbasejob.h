#ifndef STATBLOCKBASEJOB_H
#define STATBLOCKBASEJOB_H

#include <sqlite/sqlitetypes.h>

#include <util/worker/workerjob.h>

class StatBlockManager;

class StatBlockBaseJob : public WorkerJob
{
public:
    StatBlockManager *manager() const { return m_manager; }
    SqliteDb *sqliteDb() const;

    void doJob(WorkerObject *worker) override;
    void reportResult(WorkerObject *worker) override;

protected:
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
