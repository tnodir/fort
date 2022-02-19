#include "rpcmanager.h"

#include <conf/firewallconf.h>
#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/appinfomanagerrpc.h>
#include <rpc/confmanagerrpc.h>
#include <rpc/drivermanagerrpc.h>
#include <rpc/quotamanagerrpc.h>
#include <rpc/serviceinfomanagerrpc.h>
#include <rpc/statmanagerrpc.h>
#include <rpc/taskmanagerrpc.h>
#include <util/ioc/ioccontainer.h>

namespace {

inline bool sendCommandDataToClients(
        const QByteArray &commandData, const QList<ControlWorker *> &clients)
{
    bool ok = true;

    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        if (!w->sendCommandData(commandData)) {
            qWarning() << "Send command error:" << w->id() << w->errorString();
            ok = false;
        }
    }

    return ok;
}

inline bool processConfManager_confChanged(ConfManager *confManager, const QVariantList &args)
{
    if (auto cm = qobject_cast<ConfManagerRpc *>(confManager)) {
        cm->onConfChanged(args.value(0));
    }
    return true;
}

inline bool processConfManager_saveVariant(ConfManager *confManager, const QVariantList &args)
{
    return confManager->saveVariant(args.value(0));
}

inline bool processConfManager_addApp(ConfManager *confManager, const QVariantList &args)
{
    return confManager->addApp(args.value(0).toString(), args.value(1).toString(),
            args.value(2).toDateTime(), args.value(3).toInt(), args.value(4).toBool(),
            args.value(5).toBool(), args.value(6).toBool());
}

inline bool processConfManager_deleteApp(ConfManager *confManager, const QVariantList &args)
{
    return confManager->deleteApp(args.value(0).toLongLong());
}

inline bool processConfManager_updateApp(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateApp(args.value(0).toLongLong(), args.value(1).toString(),
            args.value(2).toString(), args.value(3).toDateTime(), args.value(4).toInt(),
            args.value(5).toBool(), args.value(6).toBool(), args.value(7).toBool());
}

inline bool processConfManager_updateAppBlocked(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateAppBlocked(args.value(0).toLongLong(), args.value(1).toBool());
}

inline bool processConfManager_updateAppName(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateAppName(args.value(0).toLongLong(), args.value(1).toString());
}

inline bool processConfManager_addZone(
        ConfManager *confManager, const QVariantList &args, QVariantList &resArgs)
{
    int zoneId = 0;
    const bool ok = confManager->addZone(args.value(0).toString(), args.value(1).toString(),
            args.value(2).toString(), args.value(3).toString(), args.value(4).toBool(),
            args.value(5).toBool(), zoneId);
    resArgs = { zoneId };
    return ok;
}

inline bool processConfManager_deleteZone(ConfManager *confManager, const QVariantList &args)
{
    return confManager->deleteZone(args.value(0).toLongLong());
}

inline bool processConfManager_updateZone(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateZone(args.value(0).toLongLong(), args.value(1).toString(),
            args.value(2).toString(), args.value(3).toString(), args.value(4).toString(),
            args.value(5).toBool(), args.value(6).toBool());
}

inline bool processConfManager_updateZoneName(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateZoneName(args.value(0).toLongLong(), args.value(1).toString());
}

inline bool processConfManager_updateZoneEnabled(ConfManager *confManager, const QVariantList &args)
{
    return confManager->updateZoneEnabled(args.value(0).toLongLong(), args.value(1).toBool());
}

inline bool processConfManager_checkPassword(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const bool ok = confManager->checkPassword(p.args.value(0).toString());
    if (ok && !p.worker->isClientValidated()) {
        p.worker->setIsClientValidated(true);
    }
    resArgs = { ok };
    return true;
}

inline bool processConfManagerRpcResult(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    switch (p.command) {
    case Control::Rpc_ConfManager_save:
        return processConfManager_saveVariant(confManager, p.args);
    case Control::Rpc_ConfManager_addApp:
        return processConfManager_addApp(confManager, p.args);
    case Control::Rpc_ConfManager_deleteApp:
        return processConfManager_deleteApp(confManager, p.args);
    case Control::Rpc_ConfManager_purgeApps:
        return confManager->purgeApps();
    case Control::Rpc_ConfManager_updateApp:
        return processConfManager_updateApp(confManager, p.args);
    case Control::Rpc_ConfManager_updateAppBlocked:
        return processConfManager_updateAppBlocked(confManager, p.args);
    case Control::Rpc_ConfManager_updateAppName:
        return processConfManager_updateAppName(confManager, p.args);
    case Control::Rpc_ConfManager_addZone:
        return processConfManager_addZone(confManager, p.args, resArgs);
    case Control::Rpc_ConfManager_deleteZone:
        return processConfManager_deleteZone(confManager, p.args);
    case Control::Rpc_ConfManager_updateZone:
        return processConfManager_updateZone(confManager, p.args);
    case Control::Rpc_ConfManager_updateZoneName:
        return processConfManager_updateZoneName(confManager, p.args);
    case Control::Rpc_ConfManager_updateZoneEnabled:
        return processConfManager_updateZoneEnabled(confManager, p.args);
    case Control::Rpc_ConfManager_checkPassword:
        return processConfManager_checkPassword(confManager, p, resArgs);
    default:
        return false;
    }
}

inline bool processStatManager_appStatRemoved(StatManager *statManager, const QVariantList &args)
{
    emit statManager->appStatRemoved(args.value(0).toLongLong());
    return true;
}

inline bool processStatManager_appCreated(StatManager *statManager, const QVariantList &args)
{
    emit statManager->appCreated(args.value(0).toLongLong(), args.value(1).toString());
    return true;
}

inline bool processStatManager_trafficAdded(StatManager *statManager, const QVariantList &args)
{
    emit statManager->trafficAdded(
            args.value(0).toLongLong(), args.value(1).toUInt(), args.value(2).toUInt());
    return true;
}

inline bool processStatManager_connChanged(StatManager *statManager)
{
    if (auto sm = qobject_cast<StatManagerRpc *>(statManager)) {
        sm->onConnChanged();
    }
    return true;
}

inline bool processStatManagerRpcResult(StatManager *statManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatManager_deleteStatApp:
        return statManager->deleteStatApp(p.args.value(0).toLongLong());
    case Control::Rpc_StatManager_deleteConn:
        return statManager->deleteConn(p.args.value(0).toLongLong(), p.args.value(1).toBool());
    case Control::Rpc_StatManager_deleteConnAll:
        return statManager->deleteConnAll();
    case Control::Rpc_StatManager_resetAppTrafTotals:
        return statManager->resetAppTrafTotals();
    case Control::Rpc_StatManager_clearTraffic:
        return statManager->clearTraffic();
    default:
        return false;
    }
}

inline bool serviceInfoManager_trackService(
        ServiceInfoManager *serviceInfoManager, const QVariantList &args)
{
    serviceInfoManager->trackService(args.value(0).toString());
    return true;
}

inline bool serviceInfoManager_revertService(
        ServiceInfoManager *serviceInfoManager, const QVariantList &args)
{
    serviceInfoManager->revertService(args.value(0).toString());
    return true;
}

inline bool processTaskManager_runTask(TaskManager *taskManager, const QVariantList &args)
{
    taskManager->runTask(args.value(0).toInt());
    return true;
}

inline bool processTaskManager_abortTask(TaskManager *taskManager, const QVariantList &args)
{
    taskManager->abortTask(args.value(0).toInt());
    return true;
}

inline bool processTaskManager_taskStarted(TaskManager *taskManager, const QVariantList &args)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskStarted(args.value(0).toInt());
    }
    return true;
}

inline bool processTaskManager_taskFinished(TaskManager *taskManager, const QVariantList &args)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskFinished(args.value(0).toInt());
    }
    return true;
}

}

RpcManager::RpcManager(QObject *parent) : QObject(parent) { }

void RpcManager::setUp()
{
    if (IoC<FortSettings>()->isService()) {
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
    auto appInfoManager = IoC<AppInfoManager>();

    connect(appInfoManager, &AppInfoManager::lookupFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                invokeOnClients(Control::Rpc_AppInfoManager_checkLookupFinished, { appPath });
            });
}

void RpcManager::setupConfManagerSignals()
{
    auto confManager = IoC<ConfManager>();

    connect(confManager, &ConfManager::confChanged, this, [&](bool onlyFlags) {
        const QVariant confVar = IoC<ConfManager>()->toPatchVariant(onlyFlags);
        invokeOnClients(Control::Rpc_ConfManager_confChanged, { confVar });
    });

    connect(confManager, &ConfManager::appAlerted, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appAlerted); });
    connect(confManager, &ConfManager::appChanged, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appChanged); });
    connect(confManager, &ConfManager::appUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_appUpdated); });

    connect(confManager, &ConfManager::zoneAdded, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_zoneAdded); });
    connect(confManager, &ConfManager::zoneRemoved, this,
            [&](int zoneId) { invokeOnClients(Control::Rpc_ConfManager_zoneRemoved, { zoneId }); });
    connect(confManager, &ConfManager::zoneUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfManager_zoneUpdated); });
}

void RpcManager::setupDriverManagerSignals()
{
    auto driverManager = IoC<DriverManager>();

    const auto updateClientStates = [&] {
        invokeOnClients(Control::Rpc_DriverManager_updateState, driverManager_updateState_args());
    };
    connect(driverManager, &DriverManager::errorCodeChanged, this, updateClientStates);
    connect(driverManager, &DriverManager::isDeviceOpenedChanged, this, updateClientStates);
}

void RpcManager::setupQuotaManagerSignals()
{
    auto quotaManager = IoC<QuotaManager>();

    connect(quotaManager, &QuotaManager::alert, this, [&](qint8 alertType) {
        invokeOnClients(Control::Rpc_QuotaManager_alert, { alertType });
    });
}

void RpcManager::setupStatManagerSignals()
{
    auto statManager = IoC<StatManager>();

    connect(statManager, &StatManager::trafficCleared, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_trafficCleared); });
    connect(statManager, &StatManager::appStatRemoved, this, [&](qint64 appId) {
        invokeOnClients(Control::Rpc_StatManager_appStatRemoved, { appId });
    });
    connect(statManager, &StatManager::appCreated, this, [&](qint64 appId, const QString &appPath) {
        invokeOnClients(Control::Rpc_StatManager_appCreated, { appId, appPath });
    });
    connect(statManager, &StatManager::trafficAdded, this,
            [&](qint64 unixTime, quint32 inBytes, quint32 outBytes) {
                invokeOnClients(
                        Control::Rpc_StatManager_trafficAdded, { unixTime, inBytes, outBytes });
            });
    connect(statManager, &StatManager::connChanged, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_connChanged); });
    connect(statManager, &StatManager::appTrafTotalsResetted, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_appTrafTotalsResetted); });
}

void RpcManager::setupTaskManagerSignals()
{
    auto taskManager = IoC<TaskManager>();

    connect(taskManager, &TaskManager::taskStarted, this, [&](qint8 taskType) {
        invokeOnClients(Control::Rpc_TaskManager_taskStarted, { taskType });
    });
    connect(taskManager, &TaskManager::taskFinished, this, [&](qint8 taskType) {
        invokeOnClients(Control::Rpc_TaskManager_taskFinished, { taskType });
    });
}

void RpcManager::setupClient()
{
    auto controlManager = IoC()->setUpDependency<ControlManager>();

    m_client = controlManager->newServiceClient(this);
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

bool RpcManager::invokeOnServer(Control::Command cmd, const QVariantList &args)
{
    return client()->sendCommand(cmd, args);
}

bool RpcManager::doOnServer(Control::Command cmd, const QVariantList &args, QVariantList *resArgs)
{
    if (!client()->isConnected()) {
        IoC<WindowManager>()->showErrorBox(tr("Service isn't available."));
        return false;
    }

    if (!invokeOnServer(cmd, args))
        return false;

    if (!waitResult()) {
        IoC<WindowManager>()->showErrorBox(tr("Service isn't responding."));
        return false;
    }

    if (resultCommand() != Control::Rpc_Result_Ok) {
        IoC<WindowManager>()->showErrorBox(tr("Service error."));
        return false;
    }

    if (resArgs) {
        *resArgs = resultArgs();
    }

    return true;
}

void RpcManager::invokeOnClients(Control::Command cmd, const QVariantList &args)
{
    const auto clients = IoC<ControlManager>()->clients();
    if (clients.isEmpty())
        return;

    const QByteArray buffer = ControlWorker::buildCommandData(cmd, args);
    if (buffer.isEmpty()) {
        qWarning() << "Bad RPC command to invoke:" << cmd << args;
        return;
    }

    if (!sendCommandDataToClients(buffer, clients)) {
        qWarning() << "Invoke on clients error:" << cmd << args;
    }
}

bool RpcManager::checkClientValidated(ControlWorker *w) const
{
    return !IoC<FortSettings>()->isPasswordRequired() || w->isClientValidated();
}

void RpcManager::initClientOnServer(ControlWorker *w) const
{
    w->setIsServiceClient(true);

    w->sendCommand(Control::Rpc_DriverManager_updateState, driverManager_updateState_args());
}

QVariantList RpcManager::driverManager_updateState_args() const
{
    auto driverManager = IoC<DriverManager>();

    return { driverManager->errorCode(), driverManager->isDeviceOpened() };
}

bool RpcManager::processCommandRpc(const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_Result_Ok:
    case Control::Rpc_Result_Error:
        m_resultCommand = p.command;
        m_resultArgs = p.args;
        return true;

    case Control::Rpc_RpcManager_initClient:
        initClientOnServer(p.worker);
        return true;

    default:
        return processManagerRpc(p);
    }
}

bool RpcManager::processManagerRpc(const ProcessCommandArgs &p)
{
    if (commandRequiresValidation(p.command) && !checkClientValidated(p.worker)) {
        p.errorMessage = "Client is not validated";
        return false;
    }

    switch (Control::managerByCommand(p.command)) {
    case Control::Rpc_AppInfoManager:
        return processAppInfoManagerRpc(p);

    case Control::Rpc_ConfManager:
        return processConfManagerRpc(p);

    case Control::Rpc_DriverManager:
        return processDriverManagerRpc(p);

    case Control::Rpc_QuotaManager:
        return processQuotaManagerRpc(p);

    case Control::Rpc_StatManager:
        return processStatManagerRpc(p);

    case Control::Rpc_ServiceInfoManager:
        return processServiceInfoManagerRpc(p);

    case Control::Rpc_TaskManager:
        return processTaskManagerRpc(p);

    default:
        p.errorMessage = "Unknown command";
        return false;
    }
}

bool RpcManager::processAppInfoManagerRpc(const ProcessCommandArgs &p)
{
    auto appInfoManager = IoC<AppInfoManager>();

    switch (p.command) {
    case Control::Rpc_AppInfoManager_lookupAppInfo:
        appInfoManager->lookupAppInfo(p.args.value(0).toString());
        return true;
    case Control::Rpc_AppInfoManager_checkLookupFinished:
        appInfoManager->checkLookupFinished(p.args.value(0).toString());
        return true;
    default:
        return false;
    }
}

bool RpcManager::processConfManagerRpc(const ProcessCommandArgs &p)
{
    auto confManager = IoC<ConfManager>();

    switch (p.command) {
    case Control::Rpc_ConfManager_confChanged:
        return processConfManager_confChanged(confManager, p.args);
    case Control::Rpc_ConfManager_appAlerted:
        emit confManager->appAlerted();
        return true;
    case Control::Rpc_ConfManager_appChanged:
        emit confManager->appChanged();
        return true;
    case Control::Rpc_ConfManager_appUpdated:
        emit confManager->appUpdated();
        return true;
    case Control::Rpc_ConfManager_zoneAdded:
        emit confManager->zoneAdded();
        return true;
    case Control::Rpc_ConfManager_zoneRemoved:
        emit confManager->zoneRemoved(p.args.value(0).toInt());
        return true;
    case Control::Rpc_ConfManager_zoneUpdated:
        emit confManager->zoneUpdated();
        return true;
    default: {
        QVariantList resArgs;
        const bool ok = processConfManagerRpcResult(confManager, p, resArgs);
        sendResult(p.worker, ok, resArgs);
        return true;
    }
    }
}

bool RpcManager::processDriverManagerRpc(const ProcessCommandArgs &p)
{
    auto driverManager = IoC<DriverManager>();

    switch (p.command) {
    case Control::Rpc_DriverManager_updateState:
        if (auto dm = qobject_cast<DriverManagerRpc *>(driverManager)) {
            dm->updateState(p.args.value(0).toUInt(), p.args.value(1).toBool());
        }
        return true;
    default:
        return false;
    }
}

bool RpcManager::processQuotaManagerRpc(const ProcessCommandArgs &p)
{
    auto quotaManager = IoC<QuotaManager>();

    switch (p.command) {
    case Control::Rpc_QuotaManager_alert:
        emit quotaManager->alert(p.args.value(0).toInt());
        return true;
    default:
        return false;
    }
}

bool RpcManager::processStatManagerRpc(const ProcessCommandArgs &p)
{
    auto statManager = IoC<StatManager>();

    switch (p.command) {
    case Control::Rpc_StatManager_trafficCleared:
        emit statManager->trafficCleared();
        return true;
    case Control::Rpc_StatManager_appStatRemoved:
        return processStatManager_appStatRemoved(statManager, p.args);
    case Control::Rpc_StatManager_appCreated:
        return processStatManager_appCreated(statManager, p.args);
    case Control::Rpc_StatManager_trafficAdded:
        return processStatManager_trafficAdded(statManager, p.args);
    case Control::Rpc_StatManager_connChanged:
        return processStatManager_connChanged(statManager);
    case Control::Rpc_StatManager_appTrafTotalsResetted:
        emit statManager->appTrafTotalsResetted();
        return true;
    default: {
        const bool ok = processStatManagerRpcResult(statManager, p);
        sendResult(p.worker, ok);
        return true;
    }
    }
}

bool RpcManager::processServiceInfoManagerRpc(const ProcessCommandArgs &p)
{
    auto serviceInfoManager = IoC<ServiceInfoManager>();

    switch (p.command) {
    case Control::Rpc_ServiceInfoManager_trackService:
        return serviceInfoManager_trackService(serviceInfoManager, p.args);
    case Control::Rpc_ServiceInfoManager_revertService:
        return serviceInfoManager_revertService(serviceInfoManager, p.args);
    default:
        return false;
    }
}

bool RpcManager::processTaskManagerRpc(const ProcessCommandArgs &p)
{
    auto taskManager = IoC<TaskManager>();

    switch (p.command) {
    case Control::Rpc_TaskManager_runTask:
        return processTaskManager_runTask(taskManager, p.args);
    case Control::Rpc_TaskManager_abortTask:
        return processTaskManager_abortTask(taskManager, p.args);
    case Control::Rpc_TaskManager_taskStarted:
        return processTaskManager_taskStarted(taskManager, p.args);
    case Control::Rpc_TaskManager_taskFinished:
        return processTaskManager_taskFinished(taskManager, p.args);
    default:
        return false;
    }
}
