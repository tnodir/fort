#ifndef RPCMANAGER_H
#define RPCMANAGER_H

#include <QObject>

class ControlManager;
class FortManager;
class FortSettings;

class RpcManager : public QObject
{
    Q_OBJECT

public:
    enum RpcObject : qint8 {
        Obj_AppInfoManager = 1,
        Obj_ConfManager,
        Obj_DriverManager,
        Obj_QuotaManager,
        Obj_StatManager,
        Obj_TaskManager,
    };

    explicit RpcManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ControlManager *controlManager() const;

    void initialize();

    void invokeOnServer(qint8 rpcObj, const char *member, const QVariantList &args);

private:
    void setupServerSignals();
    void setupAppInfoManagerSignals();
    void setupQuotaManagerSignals();

    void invokeOnClients(qint8 rpcObj, const char *member, const QVariantList &args);

private:
    FortManager *m_fortManager = nullptr;
};

#endif // RPCMANAGER_H
