#ifndef CONFGROUPMANAGERRPC_H
#define CONFGROUPMANAGERRPC_H

#include <conf/confgroupmanager.h>
#include <control/control_types.h>

class RpcManager;

class ConfGroupManagerRpc : public ConfGroupManager
{
    Q_OBJECT

public:
    explicit ConfGroupManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateGroup(Group &group) override;
    bool deleteGroup(quint8 groupId) override;
    bool updateGroupName(quint8 groupId, const QString &groupName) override;
    bool updateGroupEnabled(quint8 groupId, bool enabled) override;

    static QVariantList groupToVarList(const Group &group);
    static Group varListToGroup(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFGROUPMANAGERRPC_H
