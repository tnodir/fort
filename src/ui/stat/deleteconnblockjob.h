#ifndef DELETECONNBLOCKJOB_H
#define DELETECONNBLOCKJOB_H

#include "statblockbasejob.h"

class StatBlockManager;

class DeleteConnBlockJob : public StatBlockBaseJob
{
public:
    explicit DeleteConnBlockJob(qint64 connIdTo);

    qint64 connIdTo() const { return m_connIdTo; }

    StatBlockJobType jobType() const override { return JobTypeDeleteConn; }

protected:
    bool processMerge(const StatBlockBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    qint64 m_connIdTo = 0;
};

#endif // DELETECONNBLOCKJOB_H
