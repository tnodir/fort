#include "confappmanagerrpc.h"

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

namespace {

bool processConfAppManager_addOrUpdateAppPath(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->addOrUpdateAppPath(
            p.args.value(0).toString(), p.args.value(1).toBool(), p.args.value(2).toBool());
}

bool processConfAppManager_deleteAppPath(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->deleteAppPath(p.args.value(0).toString());
}

bool processConfAppManager_addOrUpdateApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    App app = ConfAppManagerRpc::varListToApp(p.args.value(0).toList());

    return confAppManager->addOrUpdateApp(app, p.args.value(1).toBool());
}

bool processConfAppManager_updateApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    App app = ConfAppManagerRpc::varListToApp(p.args);

    return confAppManager->updateApp(app);
}

bool processConfAppManager_updateAppName(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->updateAppName(p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfAppManager_deleteApps(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    QVector<qint64> appIdList;
    VariantUtil::listToVector(p.args.value(0).toList(), appIdList);

    return confAppManager->deleteApps(appIdList);
}

bool processConfAppManager_purgeApps(ConfAppManager *confAppManager,
        const ProcessCommandArgs & /*p*/, QVariantList & /*resArgs*/)
{
    return confAppManager->purgeApps();
}

bool processConfAppManager_updateAppsBlocked(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    QVector<qint64> appIdList;
    VariantUtil::listToVector(p.args.value(0).toList(), appIdList);

    return confAppManager->updateAppsBlocked(
            appIdList, p.args.value(1).toBool(), p.args.value(2).toBool());
}

using processConfAppManager_func = bool (*)(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfAppManager_func processConfAppManager_funcList[] = {
    &processConfAppManager_addOrUpdateAppPath, // Rpc_ConfAppManager_addOrUpdateAppPath,
    &processConfAppManager_deleteAppPath, // Rpc_ConfAppManager_deleteAppPath,
    &processConfAppManager_addOrUpdateApp, // Rpc_ConfAppManager_addOrUpdateApp,
    &processConfAppManager_updateApp, // Rpc_ConfAppManager_updateApp,
    &processConfAppManager_updateAppName, // Rpc_ConfAppManager_updateAppName,
    &processConfAppManager_deleteApps, // Rpc_ConfAppManager_deleteApps,
    &processConfAppManager_purgeApps, // Rpc_ConfAppManager_purgeApps,
    &processConfAppManager_updateAppsBlocked, // Rpc_ConfAppManager_updateAppsBlocked,
};

inline bool processConfAppManagerRpcResult(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfAppManager_func func = RpcManager::getProcessFunc(p.command,
            processConfAppManager_funcList, Control::Rpc_ConfAppManager_addOrUpdateAppPath,
            Control::Rpc_ConfAppManager_updateAppsBlocked);

    return func ? func(confAppManager, p, resArgs) : false;
}

}

ConfAppManagerRpc::ConfAppManagerRpc(QObject *parent) : ConfAppManager(parent) { }

bool ConfAppManagerRpc::addOrUpdateAppPath(
        const QString &appOriginPath, bool blocked, bool killProcess)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_addOrUpdateAppPath,
            { appOriginPath, blocked, killProcess });
}

bool ConfAppManagerRpc::deleteAppPath(const QString &appOriginPath)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfAppManager_deleteAppPath, { appOriginPath });
}

bool ConfAppManagerRpc::addOrUpdateApp(App &app, bool onlyUpdate)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfAppManager_addOrUpdateApp, { appToVarList(app), onlyUpdate });
}

bool ConfAppManagerRpc::updateApp(App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_updateApp, appToVarList(app));
}

bool ConfAppManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfAppManager_updateAppName, { appId, appName });
}

bool ConfAppManagerRpc::deleteApps(const QVector<qint64> &appIdList)
{
    QVariantList appIdVarList;
    VariantUtil::vectorToList(appIdList, appIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(appIdVarList));

    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_deleteApps, args);
}

bool ConfAppManagerRpc::purgeApps()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_purgeApps);
}

bool ConfAppManagerRpc::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    QVariantList appIdVarList;
    VariantUtil::vectorToList(appIdList, appIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(appIdVarList));
    args << blocked << killProcess;

    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_updateAppsBlocked, args);
}

QVariantList ConfAppManagerRpc::appToVarList(const App &app)
{
    return { app.isWildcard, app.useGroupPerm, app.applyChild, app.killChild, app.lanOnly,
        app.parked, app.logBlocked, app.logConn, app.blocked, app.killProcess, app.groupIndex,
        app.acceptZones, app.rejectZones, app.ruleId, app.appId, app.appOriginPath, app.appPath,
        app.appName, app.notes, app.scheduleAction, app.scheduleTime };
}

App ConfAppManagerRpc::varListToApp(const QVariantList &v)
{
    App app;
    app.isWildcard = v.value(0).toBool();
    app.useGroupPerm = v.value(1).toBool();
    app.applyChild = v.value(2).toBool();
    app.killChild = v.value(3).toBool();
    app.lanOnly = v.value(4).toBool();
    app.parked = v.value(5).toBool();
    app.logBlocked = v.value(6).toBool();
    app.logConn = v.value(7).toBool();
    app.blocked = v.value(8).toBool();
    app.killProcess = v.value(9).toBool();
    app.groupIndex = v.value(10).toInt();
    app.acceptZones = v.value(11).toUInt();
    app.rejectZones = v.value(12).toUInt();
    app.ruleId = v.value(13).toUInt();
    app.appId = v.value(14).toLongLong();
    app.appOriginPath = v.value(15).toString();
    app.appPath = v.value(16).toString();
    app.appName = v.value(17).toString();
    app.notes = v.value(18).toString();
    app.scheduleAction = v.value(19).toInt();
    app.scheduleTime = v.value(20).toDateTime();
    return app;
}

bool ConfAppManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confAppManager = IoC<ConfAppManager>();

    switch (p.command) {
    case Control::Rpc_ConfAppManager_appAlerted: {
        emit confAppManager->appAlerted();
        return true;
    }
    case Control::Rpc_ConfAppManager_appsChanged: {
        emit confAppManager->appsChanged();
        return true;
    }
    case Control::Rpc_ConfAppManager_appUpdated: {
        emit confAppManager->appUpdated();
        return true;
    }
    default: {
        ok = processConfAppManagerRpcResult(confAppManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

void ConfAppManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confAppManager = IoC<ConfAppManager>();

    connect(confAppManager, &ConfAppManager::appAlerted, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appAlerted); });
    connect(confAppManager, &ConfAppManager::appsChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appsChanged); });
    connect(confAppManager, &ConfAppManager::appUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appUpdated); });
}
