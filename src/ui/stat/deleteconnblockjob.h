#ifndef DELETECONNBLOCKJOB_H
#define DELETECONNBLOCKJOB_H

#include "statblockbasejob.h"

class StatBlockManager;

class DeleteConnBlockJob : public StatBlockBaseJob
{
public:
    explicit DeleteConnBlockJob(qint64 connIdTo);

    qint64 connIdTo() const { return m_connIdTo; }

protected:
    void processJob() override;
    void emitFinished() override;

    void deleteConn(qint64 connIdTo);
    void deleteConnAll();

private:
    const qint64 m_connIdTo;
};

#endif // DELETECONNBLOCKJOB_H
