#ifndef SHIFTCONNIDSJOB_H
#define SHIFTCONNIDSJOB_H

#include "deleteconnidsjob.h"

/* Deletes the connections and shifts the later ones' ids down, keeping the ids contiguous */
class ShiftConnIdsJob : public DeleteConnIdsJob
{
public:
    explicit ShiftConnIdsJob(const QVector<qint64> &connIds);

    StatConnJobType jobType() const override { return JobTypeShiftConnIds; }

protected:
    void deleteConns() override;
    void emitFinished() override;

private:
    void shiftConnIds();

    QVector<qint64> selectConnIdsAfter(qint64 connId);
    void updateConnId(qint64 connId, qint64 newConnId);
};

#endif // SHIFTCONNIDSJOB_H
