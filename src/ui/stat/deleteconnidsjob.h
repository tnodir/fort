#ifndef DELETECONNIDSJOB_H
#define DELETECONNIDSJOB_H

#include <QVector>

#include "statconnbasejob.h"

/* Deletes the connections; shifts the later ones' ids down, when the manager keeps the ids
 * contiguous */
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

    void setConnIds(const QVector<qint64> &connIds) { m_connIds = connIds; }

private:
    /* Shifts the later connections' ids, deletes the unused apps */
    void processDeletedConns();

    qint64 connIdFrom() const;

    void shiftConnIds();

    void updateConnId(qint64 connId, qint64 newConnId);

private:
    QVector<qint64> m_connIds;
};

#endif // DELETECONNIDSJOB_H
