#include "rpcmanager.h"

#include <QLoggingCategory>

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/appinfomanagerrpc.h>
#include <rpc/confappmanagerrpc.h>
#include <rpc/confmanagerrpc.h>
#include <rpc/confzonemanagerrpc.h>
#include <rpc/drivermanagerrpc.h>
#include <rpc/quotamanagerrpc.h>
#include <rpc/serviceinfomanagerrpc.h>
#include <rpc/statblockmanagerrpc.h>
#include <rpc/statmanagerrpc.h>
#include <rpc/taskmanagerrpc.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/variantutil.h>

namespace {

const QLoggingCategory LC("rpc");

void showErrorBox(const QString &text)
{
    auto windowManager = IoC<WindowManager>();

    QMetaObject::invokeMethod(
            windowManager, [=] { windowManager->showErrorBox(text); }, Qt::QueuedConnection);
}

inline bool sendCommandDataToClients(
        const QByteArray &commandData, const QList<ControlWorker *> &clients)
{
    bool ok = true;

    // XXX: OsUtil::setThreadIsBusy(true);

    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        if (!w->sendCommandData(commandData)) {
            qCWarning(LC) << "Send command error:" << w->id() << w->errorString();
            ok = false;
        }
    }

    // XXX: OsUtil::setThreadIsBusy(false);

    return ok;
}

template<typename F>
F *getProcessFunc(const ProcessCommandArgs &p, F *funcList[], int minIndex, int maxIndex)
{
    if (p.command < minIndex || p.command > maxIndex)
        return nullptr;

    const int funcIndex = p.command - minIndex;
    return funcList[funcIndex];
}

inline bool processConfManager_confChanged(ConfManager *confManager, const ProcessCommandArgs &p)
{
    if (auto cm = qobject_cast<ConfManagerRpc *>(confManager)) {
        cm->onConfChanged(p.args.value(0));
    }
    return true;
}

bool processConfManager_saveVariant(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->saveVariant(p.args.value(0));
}

bool processConfManager_exportMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->exportMasterBackup(p.args.value(0).toString());
}

bool processConfManager_importMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->importMasterBackup(p.args.value(0).toString());
}

bool processConfManager_checkPassword(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    const bool ok = confManager->checkPassword(p.args.value(0).toString());
    if (ok && !p.worker->isClientValidated()) {
        p.worker->setIsClientValidated(true);
    }
    return ok;
}

using processConfManager_func = bool (*)(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfManager_func processConfManager_funcList[] = {
    &processConfManager_saveVariant, // Rpc_ConfManager_saveVariant,
    &processConfManager_exportMasterBackup, // Rpc_ConfManager_exportMasterBackup,
    &processConfManager_importMasterBackup, // Rpc_ConfManager_importMasterBackup,
    &processConfManager_checkPassword, // Rpc_ConfManager_checkPassword,
};

inline bool processConfManagerRpcResult(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfManager_func func = getProcessFunc(p, processConfManager_funcList,
            Control::Rpc_ConfManager_saveVariant, Control::Rpc_ConfManager_checkPassword);

    return func ? func(confManager, p, resArgs) : false;
}

bool processConfAppManager_addApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->addApp(ConfAppManagerRpc::varListToApp(p.args));
}

bool processConfAppManager_deleteApps(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->deleteApps(VariantUtil::listToVector(p.args.value(0).toList()));
}

bool processConfAppManager_purgeApps(ConfAppManager *confAppManager,
        const ProcessCommandArgs & /*p*/, QVariantList & /*resArgs*/)
{
    return confAppManager->purgeApps();
}

bool processConfAppManager_updateApp(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->updateApp(ConfAppManagerRpc::varListToApp(p.args));
}

bool processConfAppManager_updateAppsBlocked(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->updateAppsBlocked(VariantUtil::listToVector(p.args.value(0).toList()),
            p.args.value(1).toBool(), p.args.value(2).toBool());
}

bool processConfAppManager_updateAppName(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confAppManager->updateAppName(p.args.value(0).toLongLong(), p.args.value(1).toString());
}

using processConfAppManager_func = bool (*)(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfAppManager_func processConfAppManager_funcList[] = {
    &processConfAppManager_addApp, // Rpc_ConfAppManager_addApp,
    &processConfAppManager_deleteApps, // Rpc_ConfAppManager_deleteApps,
    &processConfAppManager_purgeApps, // Rpc_ConfAppManager_purgeApps,
    &processConfAppManager_updateApp, // Rpc_ConfAppManager_updateApp,
    &processConfAppManager_updateAppsBlocked, // Rpc_ConfAppManager_updateAppsBlocked,
    &processConfAppManager_updateAppName, // Rpc_ConfAppManager_updateAppName,
};

inline bool processConfAppManagerRpcResult(
        ConfAppManager *confAppManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfAppManager_func func = getProcessFunc(p, processConfAppManager_funcList,
            Control::Rpc_ConfAppManager_addApp, Control::Rpc_ConfAppManager_updateAppName);

    return func ? func(confAppManager, p, resArgs) : false;
}

bool processConfZoneManager_addOrUpdateZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    Zone zone = ConfZoneManagerRpc::varListToZone(p.args);

    const bool ok = confZoneManager->addOrUpdateZone(zone);
    resArgs = { zone.zoneId };
    return ok;
}

bool processConfZoneManager_deleteZone(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->deleteZone(p.args.value(0).toLongLong());
}

bool processConfZoneManager_updateZoneName(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->updateZoneName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfZoneManager_updateZoneEnabled(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confZoneManager->updateZoneEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfZoneManager_func = bool (*)(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfZoneManager_func processConfZoneManager_funcList[] = {
    &processConfZoneManager_addOrUpdateZone, // Rpc_ConfZoneManager_addOrUpdateZone,
    &processConfZoneManager_deleteZone, // Rpc_ConfZoneManager_deleteZone,
    &processConfZoneManager_updateZoneName, // Rpc_ConfZoneManager_updateZoneName,
    &processConfZoneManager_updateZoneEnabled, // Rpc_ConfZoneManager_updateZoneEnabled,
};

inline bool processConfZoneManagerRpcResult(
        ConfZoneManager *confZoneManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfZoneManager_func func = getProcessFunc(p, processConfZoneManager_funcList,
            Control::Rpc_ConfZoneManager_addOrUpdateZone,
            Control::Rpc_ConfZoneManager_updateZoneEnabled);

    return func ? func(confZoneManager, p, resArgs) : false;
}

bool processStatManager_trafficCleared(StatManager *statManager, const ProcessCommandArgs & /*p*/)
{
    emit statManager->trafficCleared();
    return true;
}

bool processStatManager_appStatRemoved(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->appStatRemoved(p.args.value(0).toLongLong());
    return true;
}

bool processStatManager_appCreated(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->appCreated(p.args.value(0).toLongLong(), p.args.value(1).toString());
    return true;
}

bool processStatManager_trafficAdded(StatManager *statManager, const ProcessCommandArgs &p)
{
    emit statManager->trafficAdded(
            p.args.value(0).toLongLong(), p.args.value(1).toUInt(), p.args.value(2).toUInt());
    return true;
}

bool processStatManager_appTrafTotalsResetted(
        StatManager *statManager, const ProcessCommandArgs & /*p*/)
{
    emit statManager->appTrafTotalsResetted();
    return true;
}

using processStatManagerSignal_func = bool (*)(
        StatManager *statManager, const ProcessCommandArgs &p);

static processStatManagerSignal_func processStatManagerSignal_funcList[] = {
    &processStatManager_trafficCleared, // Rpc_StatManager_trafficCleared,
    &processStatManager_appStatRemoved, // Rpc_StatManager_appStatRemoved,
    &processStatManager_appCreated, // Rpc_StatManager_appCreated,
    &processStatManager_trafficAdded, // Rpc_StatManager_trafficAdded,
    &processStatManager_appTrafTotalsResetted, // Rpc_StatManager_appTrafTotalsResetted,
};

inline bool processStatManagerRpcSignal(StatManager *statManager, const ProcessCommandArgs &p)
{
    const processStatManagerSignal_func func = getProcessFunc(p, processStatManagerSignal_funcList,
            Control::Rpc_StatManager_trafficCleared,
            Control::Rpc_StatManager_appTrafTotalsResetted);

    return func ? func(statManager, p) : false;
}

inline bool processStatManagerRpcResult(StatManager *statManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatManager_deleteStatApp:
        return statManager->deleteStatApp(p.args.value(0).toLongLong());
    case Control::Rpc_StatManager_resetAppTrafTotals:
        return statManager->resetAppTrafTotals();
    case Control::Rpc_StatManager_clearTraffic:
        return statManager->clearTraffic();
    default:
        return false;
    }
}

inline bool processStatBlockManagerRpcResult(
        StatBlockManager *statBlockManager, const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_StatBlockManager_deleteConn:
        statBlockManager->deleteConn(p.args.value(0).toLongLong());
        return true;
    default:
        return false;
    }
}

inline bool serviceInfoManager_trackService(
        ServiceInfoManager *serviceInfoManager, const ProcessCommandArgs &p)
{
    serviceInfoManager->trackService(p.args.value(0).toString());
    return true;
}

inline bool serviceInfoManager_revertService(
        ServiceInfoManager *serviceInfoManager, const ProcessCommandArgs &p)
{
    serviceInfoManager->revertService(p.args.value(0).toString());
    return true;
}

inline bool processTaskManager_runTask(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    taskManager->runTask(p.args.value(0).toInt());
    return true;
}

inline bool processTaskManager_abortTask(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    taskManager->abortTask(p.args.value(0).toInt());
    return true;
}

inline bool processTaskManager_taskStarted(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskStarted(p.args.value(0).toInt());
    }
    return true;
}

inline bool processTaskManager_taskFinished(TaskManager *taskManager, const ProcessCommandArgs &p)
{
    if (auto tm = qobject_cast<TaskManagerRpc *>(taskManager)) {
        tm->onTaskFinished(p.args.value(0).toInt());
    }
    return true;
}

bool processAppInfoManagerRpc(const ProcessCommandArgs &p, QVariantList & /*resArgs*/,
        bool & /*ok*/, bool & /*isSendResult*/)
{
    auto appInfoManager = IoC<AppInfoManager>();

    switch (p.command) {
    case Control::Rpc_AppInfoManager_lookupAppInfo:
        appInfoManager->lookupAppInfo(p.args.value(0).toString());
        return true;
    case Control::Rpc_AppInfoManager_checkLookupInfoFinished:
        appInfoManager->checkLookupInfoFinished(p.args.value(0).toString());
        return true;
    default:
        return false;
    }
}

bool processConfManagerRpc(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confManager = IoC<ConfManager>();

    switch (p.command) {
    case Control::Rpc_ConfManager_confChanged:
        return processConfManager_confChanged(confManager, p);
    default: {
        ok = processConfManagerRpcResult(confManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

bool processConfAppManagerRpc(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confAppManager = IoC<ConfAppManager>();

    switch (p.command) {
    case Control::Rpc_ConfAppManager_appAlerted:
        emit confAppManager->appAlerted();
        return true;
    case Control::Rpc_ConfAppManager_appChanged:
        emit confAppManager->appChanged();
        return true;
    case Control::Rpc_ConfAppManager_appUpdated:
        emit confAppManager->appUpdated();
        return true;
    default: {
        ok = processConfAppManagerRpcResult(confAppManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

bool processConfZoneManagerRpc(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confZoneManager = IoC<ConfZoneManager>();

    switch (p.command) {
    case Control::Rpc_ConfZoneManager_zoneAdded:
        emit confZoneManager->zoneAdded();
        return true;
    case Control::Rpc_ConfZoneManager_zoneRemoved:
        emit confZoneManager->zoneRemoved(p.args.value(0).toInt());
        return true;
    case Control::Rpc_ConfZoneManager_zoneUpdated:
        emit confZoneManager->zoneUpdated();
        return true;
    default: {
        ok = processConfZoneManagerRpcResult(confZoneManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

bool processDriverManagerRpc(const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool & /*ok*/,
        bool & /*isSendResult*/)
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

bool processQuotaManagerRpc(const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool & /*ok*/,
        bool & /*isSendResult*/)
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

bool processStatManagerRpc(
        const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool &ok, bool &isSendResult)
{
    auto statManager = IoC<StatManager>();

    switch (p.command) {
    case Control::Rpc_StatManager_trafficCleared:
    case Control::Rpc_StatManager_appStatRemoved:
    case Control::Rpc_StatManager_appCreated:
    case Control::Rpc_StatManager_trafficAdded:
    case Control::Rpc_StatManager_appTrafTotalsResetted:
        return processStatManagerRpcSignal(statManager, p);

    default: {
        ok = processStatManagerRpcResult(statManager, p);
        isSendResult = true;
        return true;
    }
    }
}

bool processStatBlockManagerRpc(
        const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool &ok, bool &isSendResult)
{
    auto statBlockManager = IoC<StatBlockManager>();

    switch (p.command) {
    case Control::Rpc_StatBlockManager_connChanged:
        emit statBlockManager->connChanged();
        return true;
    default: {
        ok = processStatBlockManagerRpcResult(statBlockManager, p);
        isSendResult = true;
        return true;
    }
    }
}

bool processServiceInfoManagerRpc(const ProcessCommandArgs &p, QVariantList & /*resArgs*/,
        bool & /*ok*/, bool & /*isSendResult*/)
{
    auto serviceInfoManager = IoC<ServiceInfoManager>();

    switch (p.command) {
    case Control::Rpc_ServiceInfoManager_trackService:
        return serviceInfoManager_trackService(serviceInfoManager, p);
    case Control::Rpc_ServiceInfoManager_revertService:
        return serviceInfoManager_revertService(serviceInfoManager, p);
    default:
        return false;
    }
}

bool processTaskManagerRpc(const ProcessCommandArgs &p, QVariantList & /*resArgs*/, bool & /*ok*/,
        bool & /*isSendResult*/)
{
    auto taskManager = IoC<TaskManager>();

    switch (p.command) {
    case Control::Rpc_TaskManager_runTask:
        return processTaskManager_runTask(taskManager, p);
    case Control::Rpc_TaskManager_abortTask:
        return processTaskManager_abortTask(taskManager, p);
    case Control::Rpc_TaskManager_taskStarted:
        return processTaskManager_taskStarted(taskManager, p);
    case Control::Rpc_TaskManager_taskFinished:
        return processTaskManager_taskFinished(taskManager, p);
    case Control::Rpc_TaskManager_appVersionDownloaded:
        emit taskManager->appVersionDownloaded(p.args[0].toString());
        return true;
    case Control::Rpc_TaskManager_zonesDownloaded:
        emit taskManager->zonesDownloaded(p.args[0].toStringList());
        return true;
    default:
        return false;
    }
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

void RpcManager::tearDown()
{
    closeClient();
}

void RpcManager::setupServerSignals()
{
    setupAppInfoManagerSignals();
    setupConfManagerSignals();
    setupConfAppManagerSignals();
    setupConfZoneManagerSignals();
    setupDriverManagerSignals();
    setupQuotaManagerSignals();
    setupStatManagerSignals();
    setupStatBlockManagerSignals();
    setupTaskManagerSignals();
}

void RpcManager::setupAppInfoManagerSignals()
{
    auto appInfoManager = IoC<AppInfoManager>();

    connect(appInfoManager, &AppInfoManager::lookupInfoFinished, this,
            [&](const QString &appPath, const AppInfo & /*appInfo*/) {
                invokeOnClients(Control::Rpc_AppInfoManager_checkLookupInfoFinished, { appPath });
            });
}

void RpcManager::setupConfManagerSignals()
{
    auto confManager = IoC<ConfManager>();

    connect(confManager, &ConfManager::confChanged, this, [&](bool onlyFlags) {
        const QVariant confVar = IoC<ConfManager>()->toPatchVariant(onlyFlags);
        invokeOnClients(Control::Rpc_ConfManager_confChanged, { confVar });
    });
}

void RpcManager::setupConfAppManagerSignals()
{
    auto confAppManager = IoC<ConfAppManager>();

    connect(confAppManager, &ConfAppManager::appAlerted, this,
            [&] { invokeOnClients(Control::Rpc_ConfAppManager_appAlerted); });
    connect(confAppManager, &ConfAppManager::appChanged, this,
            [&] { invokeOnClients(Control::Rpc_ConfAppManager_appChanged); });
    connect(confAppManager, &ConfAppManager::appUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfAppManager_appUpdated); });
}

void RpcManager::setupConfZoneManagerSignals()
{
    auto confZoneManager = IoC<ConfZoneManager>();

    connect(confZoneManager, &ConfZoneManager::zoneAdded, this,
            [&] { invokeOnClients(Control::Rpc_ConfZoneManager_zoneAdded); });
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, this, [&](int zoneId) {
        invokeOnClients(Control::Rpc_ConfZoneManager_zoneRemoved, { zoneId });
    });
    connect(confZoneManager, &ConfZoneManager::zoneUpdated, this,
            [&] { invokeOnClients(Control::Rpc_ConfZoneManager_zoneUpdated); });
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
    connect(statManager, &StatManager::appTrafTotalsResetted, this,
            [&] { invokeOnClients(Control::Rpc_StatManager_appTrafTotalsResetted); });
}

void RpcManager::setupStatBlockManagerSignals()
{
    auto statBlockManager = IoC<StatBlockManager>();

    connect(statBlockManager, &StatBlockManager::connChanged, this,
            [&] { invokeOnClients(Control::Rpc_StatBlockManager_connChanged); });
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

    connect(taskManager, &TaskManager::appVersionDownloaded, this, [&](const QString &version) {
        invokeOnClients(Control::Rpc_TaskManager_appVersionDownloaded, { version });
    });
    connect(taskManager, &TaskManager::zonesDownloaded, this, [&](const QStringList &zoneNames) {
        invokeOnClients(Control::Rpc_TaskManager_zonesDownloaded, { zoneNames });
    });
}

void RpcManager::setupClient()
{
    auto controlManager = IoC()->setUpDependency<ControlManager>();

    m_client = controlManager->newServiceClient(this);

    connect(client(), &ControlWorker::connected, this,
            [&] { invokeOnServer(Control::Rpc_RpcManager_initClient); });

    client()->setIsTryReconnect(true);
    client()->reconnectToServer();
}

void RpcManager::closeClient()
{
    if (!client())
        return;

    client()->setIsTryReconnect(false);
    client()->close();
}

bool RpcManager::waitResult()
{
    m_resultCommand = Control::CommandNone;

    int waitCount = 3;
    do {
        if (!client()->waitForRead()) {
            if (--waitCount <= 0)
                return false;
        }
    } while (m_resultCommand == Control::CommandNone);

    return true;
}

void RpcManager::sendResult(ControlWorker *w, bool ok, const QVariantList &args)
{
    // DBG: qCDebug(LC) << "Send Result to Client: id:" << w->id() << ok << args.size();

    w->sendCommand(ok ? Control::Rpc_Result_Ok : Control::Rpc_Result_Error, args);
}

bool RpcManager::invokeOnServer(Control::Command cmd, const QVariantList &args)
{
    if (!client()->isConnected() && !client()->reconnectToServer()) {
        showErrorBox(tr("Service isn't available."));
        return false;
    }

    return client()->sendCommand(cmd, args);
}

bool RpcManager::doOnServer(Control::Command cmd, const QVariantList &args, QVariantList *resArgs)
{
    if (!invokeOnServer(cmd, args))
        return false;

    if (!waitResult()) {
        showErrorBox(tr("Service isn't responding."));
        return false;
    }

    if (resultCommand() != Control::Rpc_Result_Ok)
        return false;

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
        qCWarning(LC) << "Bad RPC command to invoke:" << cmd << args;
        return;
    }

    // DBG: qCDebug(LC) << "Invoke On Clients:" << cmd << args.size() << clients.size();

    if (!sendCommandDataToClients(buffer, clients)) {
        qCWarning(LC) << "Invoke on clients error:" << cmd << args;
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

using processManager_func = bool (*)(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

static processManager_func processManager_funcList[] = {
    &processAppInfoManagerRpc, // Control::Rpc_AppInfoManager,
    &processConfManagerRpc, // Control::Rpc_ConfManager,
    &processConfAppManagerRpc, // Control::Rpc_ConfAppManager,
    &processConfZoneManagerRpc, // Control::Rpc_ConfZoneManager,
    &processDriverManagerRpc, // Control::Rpc_DriverManager,
    &processQuotaManagerRpc, // Control::Rpc_QuotaManager,
    &processStatManagerRpc, // Control::Rpc_StatManager,
    &processStatBlockManagerRpc, // Control::Rpc_StatBlockManager,
    &processServiceInfoManagerRpc, // Control::Rpc_ServiceInfoManager,
    &processTaskManagerRpc, // Control::Rpc_TaskManager,
};

bool RpcManager::processManagerRpc(const ProcessCommandArgs &p)
{
    if (commandRequiresValidation(p.command) && !checkClientValidated(p.worker)) {
        p.errorMessage = "Client is not validated";
        sendResult(p.worker, false);
        return false;
    }

    const Control::RpcManager rpcManager = Control::managerByCommand(p.command);

    if (rpcManager < Control::Rpc_AppInfoManager || rpcManager > Control::Rpc_TaskManager) {
        p.errorMessage = "Unknown command";
        return false;
    }

    const int funcIndex = rpcManager - Control::Rpc_AppInfoManager;
    const processManager_func func = processManager_funcList[funcIndex];

    QVariantList resArgs;
    bool ok;
    bool isSendResult = false;

    if (!func(p, resArgs, ok, isSendResult))
        return false;

    if (isSendResult) {
        sendResult(p.worker, ok, resArgs);
    }

    return true;
}
