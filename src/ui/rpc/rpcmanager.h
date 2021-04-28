#ifndef RPCMANAGER_H
#define RPCMANAGER_H

#include <QObject>
#include <QVariant>

#include "../control/control.h"

class ControlManager;
class ControlWorker;
class FortManager;
class FortSettings;

class RpcManager : public QObject
{
    Q_OBJECT

public:
    explicit RpcManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ControlManager *controlManager() const;

    void initialize();

    bool processCommandRpc(Control::RpcObject rpcObj, int methodIndex, const QVariantList &args,
            QString &errorMessage);

    void invokeOnServer(Control::RpcObject rpcObj, int methodIndex, const QVariantList &args);

private:
    void setupServerSignals();
    void setupAppInfoManagerSignals();
    void setupQuotaManagerSignals();

    void invokeOnClients(Control::RpcObject rpcObj, int methodIndex, const QVariantList &args);

    QObject *getRpcObject(Control::RpcObject rpcObj) const;

private:
    FortManager *m_fortManager = nullptr;

    ControlWorker *m_client = nullptr;
};

#endif // RPCMANAGER_H
