#ifndef DELETECONNIDSJOB_H
#define DELETECONNIDSJOB_H

#include <QVector>

#include "statconnbasejob.h"

class DeleteConnIdsJob : public StatConnBaseJob
{
public:
    explicit DeleteConnIdsJob(const QVector<qint64> &connIds);

    const QVector<qint64> &connIds() const { return m_connIds; }

    StatConnJobType jobType() const override { return JobTypeDeleteConnIds; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

    virtual void deleteConns();

private:
    QVector<qint64> m_connIds;
};

#endif // DELETECONNIDSJOB_H
