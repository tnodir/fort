#ifndef LOGMANAGERRPC_H
#define LOGMANAGERRPC_H

#include "../log/logmanager.h"

class RpcManager;

class LogManagerRpc : public LogManager
{
    Q_OBJECT

public:
    explicit LogManagerRpc(FortManager *fortManager, QObject *parent = nullptr);

    RpcManager *rpcManager() const;

    void setActive(bool active) override;

    void initialize() override;

public slots:
    void close() override;
};

#endif // LOGMANAGERRPC_H
