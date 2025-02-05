#ifndef DELETECONNJOB_H
#define DELETECONNJOB_H

#include "statconnbasejob.h"

class StatConnManager;

class DeleteConnJob : public StatConnBaseJob
{
public:
    explicit DeleteConnJob(qint64 connIdTo);

    qint64 connIdTo() const { return m_connIdTo; }

    StatConnJobType jobType() const override { return JobTypeDeleteConn; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    qint64 m_connIdTo = 0;
};

#endif // DELETECONNJOB_H
