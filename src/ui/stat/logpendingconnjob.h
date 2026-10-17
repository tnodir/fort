#ifndef LOGPENDINGCONNJOB_H
#define LOGPENDINGCONNJOB_H

#include "logconnjob.h"

class LogPendingConnJob : public LogConnJob
{
public:
    explicit LogPendingConnJob(const LogEntryConn &entry);

protected:
    enum PendingConnParam : qint8 {
        ConnFlowId = ConnParamEnd,
        ConnAppAsked,
    };

    /* With the connection's flow id to allow or block it, and who asked it */
    const char *sqlInsertConn() const override;
    void bindConn(SqliteStmt *stmt, const LogEntryConn &entry, qint64 appId,
            qint64 inheritAppId) const override;
};

#endif // LOGPENDINGCONNJOB_H
