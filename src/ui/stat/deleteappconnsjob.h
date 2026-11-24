#ifndef DELETEAPPCONNSJOB_H
#define DELETEAPPCONNSJOB_H

#include "deleteconnidsjob.h"

/* Deletes all the program's connections, including the inherited ones, by its id or path */
class DeleteAppConnsJob : public DeleteConnIdsJob
{
public:
    explicit DeleteAppConnsJob(qint64 confAppId, const QString &appPath);

    qint64 confAppId() const { return m_confAppId; }
    const QString &appPath() const { return m_appPath; }

    StatConnJobType jobType() const override { return JobTypeDeleteAppConns; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;

    void deleteConns() override;

private:
    qint64 m_confAppId = 0;

    QString m_appPath;
};

#endif // DELETEAPPCONNSJOB_H
