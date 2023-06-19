#ifndef SERVICEINFOMANAGERRPC_H
#define SERVICEINFOMANAGERRPC_H

#include <manager/serviceinfomanager.h>

class ServiceInfoManagerRpc : public ServiceInfoManager
{
    Q_OBJECT

public:
    explicit ServiceInfoManagerRpc(QObject *parent = nullptr);

public slots:
    void trackService(const QString &serviceName) override;
    void revertService(const QString &serviceName) override;

protected:
    void setupServiceListMonitor() override { }
};

#endif // SERVICEINFOMANAGERRPC_H
