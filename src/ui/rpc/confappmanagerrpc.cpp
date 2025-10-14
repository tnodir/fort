#include "confappmanagerrpc.h"

#include <conf/app.h>
#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

bool processConfAppManager_addOrUpdateAppPath(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confAppManager->addOrUpdateAppPath(
            p.args.value(0).toString(), p.args.value(1).toBool(), p.args.value(2).toBool());
}

bool processConfAppManager_deleteAppPath(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confAppManager->deleteAppPath(p.args.value(0).toString());
}

bool processConfAppManager_addOrUpdateApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    App app = ConfAppManagerRpc::varListToApp(p.args.value(0).toList());

    return confAppManager->addOrUpdateApp(app, p.args.value(1).toBool());
}

bool processConfAppManager_updateApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    App app = ConfAppManagerRpc::varListToApp(p.args);

    return confAppManager->updateApp(app);
}

bool processConfAppManager_updateAppName(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confAppManager->updateAppName(p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfAppManager_deleteApps(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    QVector<qint64> appIdList;
    VariantUtil::listToVector(p.args.value(0).toList(), appIdList);

    return confAppManager->deleteApps(appIdList);
}

bool processConfAppManager_clearAlerts(ConfAppManager *confAppManager,
        const ProcessCommandArgs & /*p*/, ProcessCommandResult & /*r*/)
{
    return confAppManager->clearAlerts();
}

bool processConfAppManager_purgeApps(ConfAppManager *confAppManager,
        const ProcessCommandArgs & /*p*/, ProcessCommandResult & /*r*/)
{
    return confAppManager->purgeApps();
}

bool processConfAppManager_updateAppsBlocked(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    QVector<qint64> appIdList;
    VariantUtil::listToVector(p.args.value(0).toList(), appIdList);

    return confAppManager->updateAppsBlocked(
            appIdList, p.args.value(1).toBool(), p.args.value(2).toBool());
}

bool processConfAppManager_updateAppsTimer(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    QVector<qint64> appIdList;
    VariantUtil::listToVector(p.args.value(0).toList(), appIdList);

    return confAppManager->updateAppsTimer(appIdList, p.args.value(1).toInt());
}

bool processConfAppManager_importAppsBackup(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confAppManager->importAppsBackup(p.args.value(0).toString());
}

bool processConfAppManager_updateDriverConf(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confAppManager->updateDriverConf(p.args.value(0).toBool());
}

using processConfAppManager_func = bool (*)(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfAppManager_func processConfAppManager_funcList[] = {
    &processConfAppManager_addOrUpdateAppPath, // Rpc_ConfAppManager_addOrUpdateAppPath,
    &processConfAppManager_deleteAppPath, // Rpc_ConfAppManager_deleteAppPath,
    &processConfAppManager_addOrUpdateApp, // Rpc_ConfAppManager_addOrUpdateApp,
    &processConfAppManager_updateApp, // Rpc_ConfAppManager_updateApp,
    &processConfAppManager_updateAppName, // Rpc_ConfAppManager_updateAppName,
    &processConfAppManager_deleteApps, // Rpc_ConfAppManager_deleteApps,
    &processConfAppManager_clearAlerts, // Rpc_ConfAppManager_clearAlerts,
    &processConfAppManager_purgeApps, // Rpc_ConfAppManager_purgeApps,
    &processConfAppManager_updateAppsBlocked, // Rpc_ConfAppManager_updateAppsBlocked,
    &processConfAppManager_updateAppsTimer, // Rpc_ConfAppManager_updateAppsTimer,
    &processConfAppManager_importAppsBackup, // Rpc_ConfAppManager_importAppsBackup,
    &processConfAppManager_updateDriverConf, // Rpc_ConfAppManager_updateDriverConf,
};

inline bool processConfAppManagerRpcResult(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfAppManager_func func = RpcManager::getProcessFunc(p.command,
            processConfAppManager_funcList, Control::Rpc_ConfAppManager_addOrUpdateAppPath,
            Control::Rpc_ConfAppManager_updateDriverConf);

    return func ? func(confAppManager, p, r) : false;
}

}

ConfAppManagerRpc::ConfAppManagerRpc(QObject *parent) : ConfAppManager(parent) { }

bool ConfAppManagerRpc::addOrUpdateAppPath(
        const QString &appOriginPath, bool blocked, bool killProcess)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_addOrUpdateAppPath,
            { appOriginPath, blocked, killProcess });
}

bool ConfAppManagerRpc::deleteAppPath(const QString &appOriginPath)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_deleteAppPath, { appOriginPath });
}

bool ConfAppManagerRpc::addOrUpdateApp(App &app, bool onlyUpdate)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfAppManager_addOrUpdateApp, { appToVarList(app), onlyUpdate });
}

bool ConfAppManagerRpc::updateApp(App &app)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_updateApp, appToVarList(app));
}

bool ConfAppManagerRpc::updateAppName(qint64 appId, const QString &appName)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_updateAppName, { appId, appName });
}

bool ConfAppManagerRpc::deleteApps(const QVector<qint64> &appIdList)
{
    QVariantList appIdVarList;
    VariantUtil::vectorToList(appIdList, appIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(appIdVarList));

    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_deleteApps, args);
}

bool ConfAppManagerRpc::clearAlerts()
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_clearAlerts);
}

bool ConfAppManagerRpc::purgeApps()
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_purgeApps);
}

bool ConfAppManagerRpc::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    QVariantList appIdVarList;
    VariantUtil::vectorToList(appIdList, appIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(appIdVarList));
    args << blocked << killProcess;

    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_updateAppsBlocked, args);
}

bool ConfAppManagerRpc::updateAppsTimer(const QVector<qint64> &appIdList, int minutes)
{
    QVariantList appIdVarList;
    VariantUtil::vectorToList(appIdList, appIdVarList);

    QVariantList args;
    VariantUtil::addToList(args, QVariant(appIdVarList));
    args << minutes;

    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_updateAppsTimer, args);
}

bool ConfAppManagerRpc::importAppsBackup(const QString &path)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_importAppsBackup, { path });
}

bool ConfAppManagerRpc::updateDriverConf(bool onlyFlags)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfAppManager_updateDriverConf, { onlyFlags });
}

QVariantList ConfAppManagerRpc::appToVarList(const App &app)
{
    return { app.isWildcard, app.applyParent, app.applyChild, app.applySpecChild, app.killChild,
        app.lanOnly, app.parked, app.logStat, app.logAllowedConn, app.logBlockedConn, app.blocked,
        app.killProcess, app.groupIndex, app.zones.accept_mask, app.zones.reject_mask, app.ruleId,
        app.appId, app.appOriginPath, app.appPath, app.iconPath, app.appName, app.notes,
        app.scheduleAction, app.scheduleTime };
}

App ConfAppManagerRpc::varListToApp(const QVariantList &v)
{
    App app;
    app.isWildcard = v.value(0).toBool();
    app.applyParent = v.value(1).toBool();
    app.applyChild = v.value(2).toBool();
    app.applySpecChild = v.value(3).toBool();
    app.killChild = v.value(4).toBool();
    app.lanOnly = v.value(5).toBool();
    app.parked = v.value(6).toBool();
    app.logStat = v.value(7).toBool();
    app.logAllowedConn = v.value(8).toBool();
    app.logBlockedConn = v.value(9).toBool();
    app.blocked = v.value(10).toBool();
    app.killProcess = v.value(11).toBool();
    app.groupIndex = v.value(12).toInt();
    app.zones.accept_mask = v.value(13).toUInt();
    app.zones.reject_mask = v.value(14).toUInt();
    app.ruleId = v.value(15).toUInt();
    app.appId = v.value(16).toLongLong();
    app.appOriginPath = v.value(17).toString();
    app.appPath = v.value(18).toString();
    app.iconPath = v.value(19).toString();
    app.appName = v.value(20).toString();
    app.notes = v.value(21).toString();
    app.scheduleAction = v.value(22).toInt();
    app.scheduleTime = v.value(23).toDateTime();
    return app;
}

bool ConfAppManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confAppManager = Fort::confAppManager();

    switch (p.command) {
    case Control::Rpc_ConfAppManager_appAlerted: {
        const bool alerted = p.args.value(0).toBool();

        emit confAppManager->appAlerted(alerted);
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
    case Control::Rpc_ConfAppManager_appDeleted: {
        const qint64 appId = p.args.value(0).toLongLong();

        emit confAppManager->appDeleted(appId);
        return true;
    }
    default: {
        r.ok = processConfAppManagerRpcResult(confAppManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfAppManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confAppManager = Fort::confAppManager();

    connect(confAppManager, &ConfAppManager::appAlerted, rpcManager, [=](bool alerted) {
        rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appAlerted, { alerted });
    });
    connect(confAppManager, &ConfAppManager::appsChanged, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appsChanged); });
    connect(confAppManager, &ConfAppManager::appUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appUpdated); });
    connect(confAppManager, &ConfAppManager::appDeleted, rpcManager, [=](qint64 appId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfAppManager_appDeleted, { appId });
    });
}
