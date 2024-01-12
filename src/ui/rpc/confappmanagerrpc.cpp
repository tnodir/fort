#include "confappmanagerrpc.h"

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

ConfAppManagerRpc::ConfAppManagerRpc(QObject *parent) : ConfAppManager(parent) { }

bool ConfAppManagerRpc::addApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_addApp, appToVarList(app));
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

bool ConfAppManagerRpc::updateApp(const App &app)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfAppManager_updateApp, appToVarList(app));
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

bool ConfAppManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfAppManager_updateAppName, { appId, appName });
}

QVariantList ConfAppManagerRpc::appToVarList(const App &app)
{
    return { app.isWildcard, app.useGroupPerm, app.applyChild, app.killChild, app.lanOnly,
        app.logBlocked, app.logConn, app.blocked, app.killProcess, app.groupIndex, app.acceptZones,
        app.rejectZones, app.appId, app.appOriginPath, app.appPath, app.appName, app.endTime };
}

App ConfAppManagerRpc::varListToApp(const QVariantList &v)
{
    App app;
    app.isWildcard = v.value(0).toBool();
    app.useGroupPerm = v.value(1).toBool();
    app.applyChild = v.value(2).toBool();
    app.killChild = v.value(3).toBool();
    app.lanOnly = v.value(4).toBool();
    app.logBlocked = v.value(5).toBool();
    app.logConn = v.value(6).toBool();
    app.blocked = v.value(7).toBool();
    app.killProcess = v.value(8).toBool();
    app.groupIndex = v.value(9).toInt();
    app.acceptZones = v.value(10).toUInt();
    app.rejectZones = v.value(11).toUInt();
    app.appId = v.value(12).toLongLong();
    app.appOriginPath = v.value(13).toString();
    app.appPath = v.value(14).toString();
    app.appName = v.value(15).toString();
    app.endTime = v.value(16).toDateTime();
    return app;
}
