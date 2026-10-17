#ifndef DELETETIMEDOUTCONNSJOB_H
#define DELETETIMEDOUTCONNSJOB_H

#include <QVector>

#include "statconnbasejob.h"

class DeleteTimedOutConnsJob : public StatConnBaseJob
{
public:
    explicit DeleteTimedOutConnsJob(qint64 expireTime);

    qint64 expireTime() const { return m_expireTime; }

    StatConnJobType jobType() const override { return JobTypeDeleteTimedOutConns; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    void deleteConns();

private:
    qint64 m_expireTime = 0;

    QVector<quint64> m_flowIds;
};

#endif // DELETETIMEDOUTCONNSJOB_H
