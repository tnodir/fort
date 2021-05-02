#ifndef RPCMANAGER_H
#define RPCMANAGER_H

#include <QObject>
#include <QVariant>

#include "../control/control.h"

class AppInfoManager;
class ConfManager;
class ControlManager;
class ControlWorker;
class DriverManager;
class FortManager;
class FortSettings;
class QuotaManager;
class StatManager;
class TaskManager;

class RpcManager : public QObject
{
    Q_OBJECT

public:
    explicit RpcManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ControlManager *controlManager() const;
    AppInfoManager *appInfoManager() const;
    ConfManager *confManager() const;
    DriverManager *driverManager() const;
    QuotaManager *quotaManager() const;
    StatManager *statManager() const;
    TaskManager *taskManager() const;
    ControlWorker *client() const { return m_client; }

    void initialize();

    void invokeOnServer(Control::Command cmd, const QVariantList &args = {});

    bool processCommandRpc(ControlWorker *w, Control::Command cmd, const QVariantList &args,
            QString &errorMessage);

private:
    void setupServerSignals();
    void setupAppInfoManagerSignals();
    void setupDriverManagerSignals();
    void setupQuotaManagerSignals();

    void setupClient();

    void invokeOnClients(Control::Command cmd, const QVariantList &args);

    bool checkClientValidated(ControlWorker *w) const;

private:
    FortManager *m_fortManager = nullptr;

    ControlWorker *m_client = nullptr;
};

#endif // RPCMANAGER_H
