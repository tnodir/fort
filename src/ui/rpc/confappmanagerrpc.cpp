#include "confappmanagerrpc.h"

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

ConfAppManagerRpc::ConfAppManagerRpc(QObject *parent) : ConfAppManager(parent) { }

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
    const QVariantList appIdVarList = VariantUtil::vectorToList(appIdList);

    QVariantList args;
    VariantUtil::addToList(args, appIdVarList);

    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_deleteApps, args);
}

bool ConfAppManagerRpc::purgeApps()
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_purgeApps);
}

bool ConfAppManagerRpc::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    const QVariantList appIdVarList = VariantUtil::vectorToList(appIdList);

    QVariantList args;
    VariantUtil::addToList(args, appIdVarList);
    args << blocked << killProcess;

    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_updateAppsBlocked, args);
}

QVariantList ConfAppManagerRpc::appToVarList(const App &app)
{
    return { app.isWildcard, app.useGroupPerm, app.applyChild, app.killChild, app.lanOnly,
        app.parked, app.logBlocked, app.logConn, app.blocked, app.killProcess, app.groupIndex,
        app.acceptZones, app.rejectZones, app.appId, app.appOriginPath, app.appPath, app.appName,
        app.notes, app.endTime };
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
    app.appId = v.value(13).toLongLong();
    app.appOriginPath = v.value(14).toString();
    app.appPath = v.value(15).toString();
    app.appName = v.value(16).toString();
    app.notes = v.value(17).toString();
    app.endTime = v.value(18).toDateTime();
    return app;
}
