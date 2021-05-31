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

    Control::Command resultCommand() const { return m_resultCommand; }
    QVariantList resultArgs() const { return m_resultArgs; }

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

    bool waitResult();
    void sendResult(ControlWorker *w, bool ok, const QVariantList &args = {});

    bool invokeOnServer(Control::Command cmd, const QVariantList &args = {});
    bool doOnServer(Control::Command cmd, const QVariantList &args = {});

    bool processCommandRpc(ControlWorker *w, Control::Command cmd, const QVariantList &args,
            QString &errorMessage);

private:
    void setupServerSignals();
    void setupAppInfoManagerSignals();
    void setupConfManagerSignals();
    void setupDriverManagerSignals();
    void setupQuotaManagerSignals();
    void setupStatManagerSignals();
    void setupTaskManagerSignals();

    void setupClient();

    void invokeOnClients(Control::Command cmd, const QVariantList &args = {});

    bool checkClientValidated(ControlWorker *w) const;
    bool validateClient(ControlWorker *w, const QString &password) const;
    void initClientOnServer(ControlWorker *w) const;

    QVariantList driverManager_updateState_args() const;

    bool processManagerRpc(ControlWorker *w, Control::Command cmd, const QVariantList &args,
            QString &errorMessage);

    bool processAppInfoManagerRpc(Control::Command cmd, const QVariantList &args);
    bool processConfManagerRpc(ControlWorker *w, Control::Command cmd, const QVariantList &args);
    bool processConfManagerRpcResult(ControlWorker *w, Control::Command cmd,
            const QVariantList &args, QVariantList &resArgs);
    bool processDriverManagerRpc(Control::Command cmd, const QVariantList &args);
    bool processQuotaManagerRpc(Control::Command cmd, const QVariantList &args);
    bool processStatManagerRpc(ControlWorker *w, Control::Command cmd, const QVariantList &args);
    bool processTaskManagerRpc(Control::Command cmd, const QVariantList &args);

private:
    Control::Command m_resultCommand = Control::CommandNone;
    QVariantList m_resultArgs;

    FortManager *m_fortManager = nullptr;

    ControlWorker *m_client = nullptr;
};

#endif // RPCMANAGER_H
