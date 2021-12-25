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

    bool deleteConn(qint64 rowIdTo, bool blocked) override;
    bool deleteConnAll() override;

    bool resetAppTrafTotals() override;

public slots:
    bool clearTraffic() override;

    void onConnChanged();
};

#endif // STATMANAGERRPC_H
