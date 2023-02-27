#ifndef STATMANAGERRPC_H
#define STATMANAGERRPC_H

#include <stat/statmanager.h>

class StatManagerRpc : public StatManager
{
    Q_OBJECT

public:
    explicit StatManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void setConf(const FirewallConf * /*conf*/) override { }

    bool deleteStatApp(qint64 appId) override;

    bool resetAppTrafTotals() override;

public slots:
    bool clearTraffic() override;
};

#endif // STATMANAGERRPC_H
