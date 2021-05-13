#include "rpcmanager.h"

#include "../conf/firewallconf.h"
#include "../control/controlmanager.h"
#include "../control/controlworker.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/appinfomanagerrpc.h"
#include "../rpc/confmanagerrpc.h"
#include "../rpc/drivermanagerrpc.h"
#include "../rpc/quotamanagerrpc.h"
#include "../rpc/statmanagerrpc.h"
#include "../rpc/taskmanagerrpc.h"

RpcManager::RpcManager(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
}

FortSettings *RpcManager::settings() const
{
    return fortManager()->settings();
}

ControlManager *RpcManager::controlManager() const
{
    return fortManager()->controlManager();
}

AppInfoManager *RpcManager::appInfoManager() const
{
    return fortManager()->appInfoManager();
}

ConfManager *RpcManager::confManager() const
{
    return fortManager()->confManager();
}

DriverManager *RpcManager::driverManager() const
{
    return fortManager()->driverManager();
}

QuotaManager *RpcManager::quotaManager() const
{
    return fortManager()->quotaManager();
}

StatManager *RpcManager::statManager() const
{
    return fortManager()->statManager();
}

TaskManager *RpcManager::taskManager() const
{
    return fortManager()->taskManager();
}

void RpcManager::initialize()
{
    if (settings()->isService()) {
        setupServerSignals();
    } else {
        setupClient();
    }
}

void RpcManager::setupServerSignals()
{
    setupAppInfoManagerSignals();
    setupConfManagerSignals();
    setupDriverManagerSignals();
    setupQuotaManagerSignals();
    setupStatManagerSignals();
    setupTaskManagerSignals();
}

void RpcManager::setupAppInfoManagerSignals()
{
    connect(appInfoManager(), &AppInfoManager::lookupFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                invokeOnClients(Control::Rpc_AppInfoManager_checkLookupFinished, { appPath });
            });
}

void RpcManager::setupConfManagerSignals()
{
    connect(confManager(), &ConfManager::confChanged, this, [&](bool /*onlyFlags*/) {
        const QVariant confVar = confManager()->conf()->toVariant(true);
        invokeOnClients(Control::Rpc_ConfManager_confChanged, { confVar });
    });

    connect(confManager(), &ConfManager::appEndTimesUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appEndTimesUpdated); });
    connect(confManager(), &ConfManager::appAdded, this,
            [&](bool alerted) { invokeOnClients(Control::Rpc_ConfManager_appAdded, { alerted }); });
    connect(confManager(), &ConfManager::appRemoved, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appRemoved); });
    connect(confManager(), &ConfManager::appUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appUpdated); });

    connect(confManager(), &ConfManager::zoneAdded, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_zoneAdded); });
    connect(confManager(), &ConfManager::zoneRemoved, this,
            [&](int zoneId) { invokeOnClients(Control::Rpc_ConfManager_zoneRemoved, { zoneId }); });
    connect(confManager(), &ConfManager::zoneUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_zoneUpdated); });
}

void RpcManager::setupDriverManagerSignals()
{
    const auto updateClientStates = [&] {
        invokeOnClients(Control::Rpc_DriverManager_updateState, driverManager_updateState_args());
    };
    connect(driverManager(), &DriverManager::errorCodeChanged, this, updateClientStates);
    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, updateClientStates);
}

void RpcManager::setupQuotaManagerSignals()
{
    connect(quotaManager(), &QuotaManager::alert, this, [&](qint8 alertType) {
        invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}

void RpcManager::setupStatManagerSignals()
{
    connect(statManager(), &StatManager::trafficCleared, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_trafficCleared); });
    connect(statManager(), &StatManager::appStatRemoved, this, [&](qint64 appId) {
        invokeOnClients(Control::Rpc_StatManager_appStatRemoved, { appId });
    });
    connect(statManager(), &StatManager::appCreated, this,
            [&](qint64 appId, const QString &appPath) {
                invokeOnClients(Control::Rpc_StatManager_appCreated, { appId, appPath });
            });
    connect(statManager(), &StatManager::trafficAdded, this,
            [&](qint64 unixTime, quint32 inBytes, quint32 outBytes) {
                invokeOnClients(
                        Control::Rpc_StatManager_trafficAdded, { unixTime, inBytes, outBytes });
            });
    connect(statManager(), &StatManager::connBlockAdded, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_connBlockAdded); });
    connect(statManager(), &StatManager::connRemoved, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_connRemoved); });
    connect(statManager(), &StatManager::appTrafTotalsResetted, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_appTrafTotalsResetted); });
}

void RpcManager::setupTaskManagerSignals()
{
    connect(taskManager(), &TaskManager::taskStarted, this, [&](qint8 taskType) {
        invokeOnClients(Control::Rpc_TaskManager_taskStarted, { taskType });
    });
    connect(taskManager(), &TaskManager::taskFinished, this, [&](qint8 taskType) {
        invokeOnClients(Control::Rpc_TaskManager_taskFinished, { taskType });
    });
}

void RpcManager::setupClient()
{
    m_client = controlManager()->newServiceClient(this);
    invokeOnServer(Control::Rpc_RpcManager_initClient);
}

bool RpcManager::waitResult()
{
    m_resultCommand = Control::CommandNone;

    do {
        if (!client()->waitForRead())
            return false;
    } while (m_resultCommand == Control::CommandNone);

    return true;
}

void RpcManager::sendResult(ControlWorker *w, bool ok, const QVariantList &args)
{
    w->sendCommand(ok ? Control::Rpc_Result_Ok : Control::Rpc_Result_Error, args);
}

void RpcManager::invokeOnServer(Control::Command cmd, const QVariantList &args)
{
    client()->sendCommand(cmd, args);
}

bool RpcManager::doOnServer(Control::Command cmd, const QVariantList &args)
{
    invokeOnServer(cmd, args);

    if (!waitResult()) {
        fortManager()->showErrorBox("Service isn't responding.");
        return false;
    }

    if (resultCommand() != Control::Rpc_Result_Ok) {
        fortManager()->showErrorBox("Service error.");
        return false;
    }

    return true;
}

void RpcManager::invokeOnClients(Control::Command cmd, const QVariantList &args)
{
    const QByteArray buffer = ControlWorker::buildCommandData(cmd, args);

    const auto clients = controlManager()->clients();
    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        w->sendCommandData(buffer);
    }
}

bool RpcManager::checkClientValidated(ControlWorker *w) const
{
    return !settings()->isPasswordRequired() || w->isClientValidated();
}

void RpcManager::initClientOnServer(ControlWorker *w) const
{
    w->setIsServiceClient(true);

    w->sendCommand(Control::Rpc_DriverManager_updateState, driverManager_updateState_args());
}

QVariantList RpcManager::driverManager_updateState_args() const
{
    return { driverManager()->errorCode(), driverManager()->isDeviceOpened() };
}

bool RpcManager::processCommandRpc(
        ControlWorker *w, Control::Command cmd, const QVariantList &args, QString &errorMessage)
{
    switch (cmd) {
    case Control::Rpc_Result_Ok:
    case Control::Rpc_Result_Error:
        m_resultCommand = cmd;
        m_resultArgs = args;
        return true;

    case Control::Rpc_RpcManager_initClient:
        initClientOnServer(w);
        return true;

    case Control::Rpc_AppInfoManager_lookupAppInfo:
    case Control::Rpc_AppInfoManager_checkLookupFinished:
        return processAppInfoManagerRpc(cmd, args);

    case Control::Rpc_ConfManager_save:
    case Control::Rpc_ConfManager_addApp:
    case Control::Rpc_ConfManager_deleteApp:
    case Control::Rpc_ConfManager_updateApp:
    case Control::Rpc_ConfManager_updateAppName:
    case Control::Rpc_ConfManager_addZone:
    case Control::Rpc_ConfManager_deleteZone:
    case Control::Rpc_ConfManager_updateZone:
    case Control::Rpc_ConfManager_updateZoneName:
    case Control::Rpc_ConfManager_updateZoneEnabled:
        if (!checkClientValidated(w))
            return false;
        Q_FALLTHROUGH();
    case Control::Rpc_ConfManager_confChanged:
    case Control::Rpc_ConfManager_appEndTimesUpdated:
    case Control::Rpc_ConfManager_appAdded:
    case Control::Rpc_ConfManager_appRemoved:
    case Control::Rpc_ConfManager_appUpdated:
    case Control::Rpc_ConfManager_zoneAdded:
    case Control::Rpc_ConfManager_zoneRemoved:
    case Control::Rpc_ConfManager_zoneUpdated:
        return processConfManagerRpc(w, cmd, args);

    case Control::Rpc_DriverManager_reinstallDriver:
    case Control::Rpc_DriverManager_uninstallDriver:
        if (!checkClientValidated(w))
            return false;
        Q_FALLTHROUGH();
    case Control::Rpc_DriverManager_updateState:
        return processDriverManagerRpc(cmd, args);

    case Control::Rpc_QuotaManager_alert:
        return processQuotaManagerRpc(cmd, args);

    case Control::Rpc_StatManager_deleteStatApp:
    case Control::Rpc_StatManager_deleteConn:
    case Control::Rpc_StatManager_deleteConnAll:
    case Control::Rpc_StatManager_resetAppTrafTotals:
    case Control::Rpc_StatManager_clearTraffic:
        if (!checkClientValidated(w))
            return false;
        Q_FALLTHROUGH();
    case Control::Rpc_StatManager_trafficCleared:
    case Control::Rpc_StatManager_appStatRemoved:
    case Control::Rpc_StatManager_appCreated:
    case Control::Rpc_StatManager_trafficAdded:
    case Control::Rpc_StatManager_connBlockAdded:
    case Control::Rpc_StatManager_connRemoved:
    case Control::Rpc_StatManager_appTrafTotalsResetted:
        return processStatManagerRpc(w, cmd, args);

    case Control::Rpc_TaskManager_runTask:
    case Control::Rpc_TaskManager_abortTask:
        if (!checkClientValidated(w))
            return false;
        Q_FALLTHROUGH();
    case Control::Rpc_TaskManager_taskStarted:
    case Control::Rpc_TaskManager_taskFinished:
        return processTaskManagerRpc(cmd, args);

    default:
        errorMessage = "Unknown command";
    }

    return false;
}

bool RpcManager::processAppInfoManagerRpc(Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_AppInfoManager_lookupAppInfo:
        appInfoManager()->lookupAppInfo(args.value(0).toString());
        return true;
    case Control::Rpc_AppInfoManager_checkLookupFinished:
        appInfoManager()->checkLookupFinished(args.value(0).toString());
        return true;
    default:
        return false;
    }
}

bool RpcManager::processConfManagerRpc(
        ControlWorker *w, Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_ConfManager_save:
        sendResult(w, confManager()->saveVariant(args.value(0)));
        return true;
    case Control::Rpc_ConfManager_addApp:
        sendResult(w,
                confManager()->addApp(args.value(0).toString(), args.value(1).toString(),
                        args.value(2).toDateTime(), args.value(3).toLongLong(),
                        args.value(4).toInt(), args.value(5).toBool(), args.value(6).toBool(),
                        args.value(7).toBool()));
        return true;
    case Control::Rpc_ConfManager_deleteApp:
        sendResult(
                w, confManager()->deleteApp(args.value(0).toLongLong(), args.value(1).toString()));
        return true;
    case Control::Rpc_ConfManager_updateApp:
        sendResult(w,
                confManager()->updateApp(args.value(0).toLongLong(), args.value(1).toString(),
                        args.value(2).toString(), args.value(3).toDateTime(),
                        args.value(4).toLongLong(), args.value(5).toInt(), args.value(6).toBool(),
                        args.value(7).toBool()));
        return true;
    case Control::Rpc_ConfManager_updateAppName:
        sendResult(w,
                confManager()->updateAppName(args.value(0).toLongLong(), args.value(1).toString()));
        return true;
    case Control::Rpc_ConfManager_addZone: {
        int zoneId;
        const bool ok = confManager()->addZone(args.value(0).toString(), args.value(1).toString(),
                args.value(2).toString(), args.value(3).toString(), args.value(4).toBool(),
                args.value(5).toBool(), zoneId);
        sendResult(w, ok, { zoneId });
        return true;
    }
    case Control::Rpc_ConfManager_deleteZone:
        sendResult(w, confManager()->deleteZone(args.value(0).toLongLong()));
        return true;
    case Control::Rpc_ConfManager_updateZone:
        sendResult(w,
                confManager()->updateZone(args.value(0).toLongLong(), args.value(1).toString(),
                        args.value(2).toString(), args.value(3).toString(),
                        args.value(4).toString(), args.value(5).toBool(), args.value(6).toBool()));
        return true;
    case Control::Rpc_ConfManager_updateZoneName:
        sendResult(w,
                confManager()->updateZoneName(
                        args.value(0).toLongLong(), args.value(1).toString()));
        return true;
    case Control::Rpc_ConfManager_updateZoneEnabled:
        sendResult(w,
                confManager()->updateZoneEnabled(
                        args.value(0).toLongLong(), args.value(1).toBool()));
        return true;
    case Control::Rpc_ConfManager_confChanged:
        if (auto cm = qobject_cast<ConfManagerRpc *>(confManager())) {
            cm->onConfChanged(args.value(0));
        }
        return true;
    case Control::Rpc_ConfManager_appEndTimesUpdated:
        emit confManager()->appEndTimesUpdated();
        return true;
    case Control::Rpc_ConfManager_appAdded:
        emit confManager()->appAdded(args.value(0).toBool());
        return true;
    case Control::Rpc_ConfManager_appRemoved:
        emit confManager()->appRemoved();
        return true;
    case Control::Rpc_ConfManager_appUpdated:
        emit confManager()->appUpdated();
        return true;
    case Control::Rpc_ConfManager_zoneAdded:
        emit confManager()->zoneAdded();
        return true;
    case Control::Rpc_ConfManager_zoneRemoved:
        emit confManager()->zoneRemoved(args.value(0).toInt());
        return true;
    case Control::Rpc_ConfManager_zoneUpdated:
        emit confManager()->zoneUpdated();
        return true;
    default:
        return false;
    }
}

bool RpcManager::processDriverManagerRpc(Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_DriverManager_reinstallDriver:
        fortManager()->installDriver();
        return true;
    case Control::Rpc_DriverManager_uninstallDriver:
        fortManager()->removeDriver();
        return true;
    case Control::Rpc_DriverManager_updateState:
        if (auto dm = qobject_cast<DriverManagerRpc *>(driverManager())) {
            dm->updateState(args.value(0).toUInt(), args.value(1).toBool());
        }
        return true;
    default:
        return false;
    }
}

bool RpcManager::processQuotaManagerRpc(Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_QuotaManager_alert:
        emit quotaManager()->alert(args.value(0).toInt());
        return true;
    default:
        return false;
    }
}

bool RpcManager::processStatManagerRpc(
        ControlWorker *w, Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_StatManager_deleteStatApp:
        sendResult(w, statManager()->deleteStatApp(args.value(0).toLongLong()));
        return true;
    case Control::Rpc_StatManager_deleteConn:
        sendResult(
                w, statManager()->deleteConn(args.value(0).toLongLong(), args.value(1).toBool()));
        return true;
    case Control::Rpc_StatManager_deleteConnAll:
        sendResult(w, statManager()->deleteConnAll());
        return true;
    case Control::Rpc_StatManager_resetAppTrafTotals:
        sendResult(w, statManager()->resetAppTrafTotals());
        return true;
    case Control::Rpc_StatManager_clearTraffic:
        sendResult(w, statManager()->clearTraffic());
        return true;
    case Control::Rpc_StatManager_trafficCleared:
        emit statManager()->trafficCleared();
        return true;
    case Control::Rpc_StatManager_appStatRemoved:
        emit statManager()->appStatRemoved(args.value(0).toLongLong());
        return true;
    case Control::Rpc_StatManager_appCreated:
        emit statManager()->appCreated(args.value(0).toLongLong(), args.value(1).toString());
        return true;
    case Control::Rpc_StatManager_trafficAdded:
        emit statManager()->trafficAdded(
                args.value(0).toLongLong(), args.value(1).toUInt(), args.value(2).toUInt());
        return true;
    case Control::Rpc_StatManager_connBlockAdded:
        emit statManager()->connBlockAdded();
        return true;
    case Control::Rpc_StatManager_connRemoved:
        emit statManager()->connRemoved();
        return true;
    case Control::Rpc_StatManager_appTrafTotalsResetted:
        emit statManager()->appTrafTotalsResetted();
        return true;
    default:
        return false;
    }
}

bool RpcManager::processTaskManagerRpc(Control::Command cmd, const QVariantList &args)
{
    switch (cmd) {
    case Control::Rpc_TaskManager_runTask:
        taskManager()->runTask(args.value(0).toInt());
        return true;
    case Control::Rpc_TaskManager_abortTask:
        taskManager()->abortTask(args.value(0).toInt());
        return true;
    case Control::Rpc_TaskManager_taskStarted:
        if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager())) {
            tm->onTaskStarted(args.value(0).toInt());
        }
        return true;
    case Control::Rpc_TaskManager_taskFinished:
        if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager())) {
            tm->onTaskFinished(args.value(0).toInt());
        }
        return true;
    default:
        return false;
    }
}
