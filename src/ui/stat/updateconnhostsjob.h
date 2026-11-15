#ifndef UPDATECONNHOSTSJOB_H
#define UPDATECONNHOSTSJOB_H

#include <QString>
#include <QVector>

#include <util/net/processipkey.h>

#include "statconnbasejob.h"

/* Names the process's connections to the address, logged without a name */
class UpdateConnHostsJob : public StatConnBaseJob
{
public:
    struct ConnHost
    {
        qint64 connIdMin; // the connections after it
        ProcessIpKey key;
        QString name;
    };

    explicit UpdateConnHostsJob(const ConnHost &connHost);

    const QVector<ConnHost> &connHosts() const { return m_connHosts; }

    StatConnJobType jobType() const override { return JobTypeUpdateConnHosts; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    int updateConnHost(const ConnHost &connHost);

private:
    QVector<ConnHost> m_connHosts;
};

#endif // UPDATECONNHOSTSJOB_H
