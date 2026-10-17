#ifndef DELETEPENDINGCONNJOB_H
#define DELETEPENDINGCONNJOB_H

#include <QVector>

#include "statconnbasejob.h"

class DeletePendingConnJob : public StatConnBaseJob
{
public:
    explicit DeletePendingConnJob(const QVector<qint64> &connIds);

    const QVector<qint64> &connIds() const { return m_connIds; }

    StatConnJobType jobType() const override { return JobTypeDeletePendingConn; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    QVector<qint64> m_connIds;
};

#endif // DELETEPENDINGCONNJOB_H
