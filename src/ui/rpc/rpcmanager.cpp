#include "rpcmanager.h"

#include <QLoggingCategory>

#include <conf/firewallconf.h>
#include <conf/zone.h>
#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/appinfomanagerrpc.h>
#include <rpc/confmanagerrpc.h>
#include <rpc/drivermanagerrpc.h>
#include <rpc/quotamanagerrpc.h>
#include <rpc/serviceinfomanagerrpc.h>
#include <rpc/statblockmanagerrpc.h>
#include <rpc/statmanagerrpc.h>
#include <rpc/taskmanagerrpc.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

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

    OsUtil::setThreadIsBusy(true);

    for (ControlWorker *w : clients) {
        if (!w->isServiceClient())
            continue;

        if (!w->sendCommandData(commandData)) {
            qCWarning(LC) << "Send command error:" << w->id() << w->errorString();
            ok = false;
        }
    }

    OsUtil::setThreadIsBusy(false);

    return ok;
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

bool processConfManager_addApp(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    App app;
    app.useGroupPerm = p.args.value(0).toBool();
    app.applyChild = p.args.value(1).toBool();
    app.lanOnly = p.args.value(2).toBool();
    app.logBlocked = p.args.value(3).toBool();
    app.logConn = p.args.value(4).toBool();
    app.blocked = p.args.value(5).toBool();
    app.killProcess = p.args.value(6).toBool();
    app.groupIndex = p.args.value(7).toInt();
    app.appOriginPath = p.args.value(8).toString();
    app.appPath = p.args.value(9).toString();
    app.appName = p.args.value(10).toString();
    app.endTime = p.args.value(11).toDateTime();

    return confManager->addApp(app);
}

bool processConfManager_deleteApp(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->deleteApp(p.args.value(0).toLongLong());
}

bool processConfManager_purgeApps(
        ConfManager *confManager, const ProcessCommandArgs & /*p*/, QVariantList & /*resArgs*/)
{
    return confManager->purgeApps();
}

bool processConfManager_updateApp(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    App app;
    app.useGroupPerm = p.args.value(0).toBool();
    app.applyChild = p.args.value(1).toBool();
    app.lanOnly = p.args.value(2).toBool();
    app.logBlocked = p.args.value(3).toBool();
    app.logConn = p.args.value(4).toBool();
    app.blocked = p.args.value(5).toBool();
    app.killProcess = p.args.value(6).toBool();
    app.groupIndex = p.args.value(7).toInt();
    app.appId = p.args.value(8).toLongLong();
    app.appOriginPath = p.args.value(9).toString();
    app.appPath = p.args.value(10).toString();
    app.appName = p.args.value(11).toString();
    app.endTime = p.args.value(12).toDateTime();

    return confManager->updateApp(app);
}

bool processConfManager_updateAppBlocked(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->updateAppBlocked(
            p.args.value(0).toLongLong(), p.args.value(1).toBool(), p.args.value(2).toBool());
}

bool processConfManager_updateAppName(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->updateAppName(p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfManager_addZone(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    Zone zone;
    zone.enabled = p.args.value(0).toBool();
    zone.customUrl = p.args.value(1).toBool();
    zone.zoneName = p.args.value(2).toString();
    zone.sourceCode = p.args.value(3).toString();
    zone.url = p.args.value(4).toString();
    zone.formData = p.args.value(5).toString();

    const bool ok = confManager->addZone(zone);
    resArgs = { zone.zoneId };
    return ok;
}

bool processConfManager_deleteZone(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->deleteZone(p.args.value(0).toLongLong());
}

bool processConfManager_updateZone(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    Zone zone;
    zone.enabled = p.args.value(0).toBool();
    zone.customUrl = p.args.value(1).toBool();
    zone.zoneId = p.args.value(2).toInt();
    zone.zoneName = p.args.value(3).toString();
    zone.sourceCode = p.args.value(4).toString();
    zone.url = p.args.value(5).toString();
    zone.formData = p.args.value(6).toString();

    return confManager->updateZone(zone);
}

bool processConfManager_updateZoneName(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->updateZoneName(p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfManager_updateZoneEnabled(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->updateZoneEnabled(p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

bool processConfManager_checkPassword(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const bool ok = confManager->checkPassword(p.args.value(0).toString());
    if (ok && !p.worker->isClientValidated()) {
        p.worker->setIsClientValidated(true);
    }
    resArgs = { ok };
    return true;
}

using processConfManager_func = bool (*)(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfManager_func processConfManager_funcList[] = {
    &processConfManager_saveVariant, // Rpc_ConfManager_saveVariant,
    &processConfManager_addApp, // Rpc_ConfManager_addApp,
    &processConfManager_deleteApp, // Rpc_ConfManager_deleteApp,
    &processConfManager_purgeApps, // Rpc_ConfManager_purgeApps,
    &processConfManager_updateApp, // Rpc_ConfManager_updateApp,
    &processConfManager_updateAppBlocked, // Rpc_ConfManager_updateAppBlocked,
    &processConfManager_updateAppName, // Rpc_ConfManager_updateAppName,
    &processConfManager_addZone, // Rpc_ConfManager_addZone,
    &processConfManager_deleteZone, // Rpc_ConfManager_deleteZone,
    &processConfManager_updateZone, // Rpc_ConfManager_updateZone,
    &processConfManager_updateZoneName, // Rpc_ConfManager_updateZoneName,
    &processConfManager_updateZoneEnabled, // Rpc_ConfManager_updateZoneEnabled,
    &processConfManager_checkPassword, // Rpc_ConfManager_checkPassword,
};

inline bool processConfManagerRpcResult(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    if (p.command >= Control::Rpc_ConfManager_saveVariant
            && p.command <= Control::Rpc_ConfManager_checkPassword) {

        const int funcIndex = p.command - Control::Rpc_ConfManager_saveVariant;
        const processConfManager_func func = processConfManager_funcList[funcIndex];

        return func(confManager, p, resArgs);
    }

    return false;
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
    const int funcIndex = p.command - Control::Rpc_StatManager_trafficCleared;
    const processStatManagerSignal_func func = processStatManagerSignal_funcList[funcIndex];

    return func(statManager, p);
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
        ok = processConfManagerRpcResult(confManager, p, resArgs);
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
        if (!client()->waitForRead() && --waitCount <= 0)
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

    if (resultCommand() != Control::Rpc_Result_Ok) {
        showErrorBox(tr("Service error."));
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
        qCWarning(LC) << "Bad RPC command to invoke:" << cmd << args;
        return;
    }

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
