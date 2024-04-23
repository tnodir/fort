#include "rpcmanager.h"

#include <QLoggingCategory>

#include <conf/firewallconf.h>
#include <conf/rule.h>
#include <conf/zone.h>
#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/appinfomanagerrpc.h>
#include <rpc/autoupdatemanagerrpc.h>
#include <rpc/confappmanagerrpc.h>
#include <rpc/confmanagerrpc.h>
#include <rpc/confrulemanagerrpc.h>
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
    AppInfoManagerRpc::setupServerSignals(this);
    AutoUpdateManagerRpc::setupServerSignals(this);
    ConfManagerRpc::setupServerSignals(this);
    ConfAppManagerRpc::setupServerSignals(this);
    ConfRuleManagerRpc::setupServerSignals(this);
    ConfZoneManagerRpc::setupServerSignals(this);
    DriverManagerRpc::setupServerSignals(this);
    QuotaManagerRpc::setupServerSignals(this);
    StatManagerRpc::setupServerSignals(this);
    StatBlockManagerRpc::setupServerSignals(this);
    TaskManagerRpc::setupServerSignals(this);
}

void RpcManager::setupClient()
{
    auto controlManager = IoCDependency<ControlManager>();

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
    const auto &clients = IoC<ControlManager>()->clients();
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

    AutoUpdateManagerRpc::processInitClient(w);
    DriverManagerRpc::processInitClient(w);
}

bool RpcManager::processCommandRpc(const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Rpc_Result_Ok:
    case Control::Rpc_Result_Error: {
        m_resultCommand = p.command;
        m_resultArgs = p.args;
        return true;
    }
    case Control::Rpc_RpcManager_initClient: {
        initClientOnServer(p.worker);
        return true;
    }
    default:
        return processManagerRpc(p);
    }
}

static processManager_func processManager_funcList[] = {
    &AppInfoManagerRpc::processServerCommand, // Control::Rpc_AppInfoManager,
    &AutoUpdateManagerRpc::processServerCommand, // Control::Rpc_AutoUpdateManager,
    &ConfManagerRpc::processServerCommand, // Control::Rpc_ConfManager,
    &ConfAppManagerRpc::processServerCommand, // Control::Rpc_ConfAppManager,
    &ConfRuleManagerRpc::processServerCommand, // Control::Rpc_ConfRuleManager,
    &ConfZoneManagerRpc::processServerCommand, // Control::Rpc_ConfZoneManager,
    &DriverManagerRpc::processServerCommand, // Control::Rpc_DriverManager,
    &QuotaManagerRpc::processServerCommand, // Control::Rpc_QuotaManager,
    &StatManagerRpc::processServerCommand, // Control::Rpc_StatManager,
    &StatBlockManagerRpc::processServerCommand, // Control::Rpc_StatBlockManager,
    &ServiceInfoManagerRpc::processServerCommand, // Control::Rpc_ServiceInfoManager,
    &TaskManagerRpc::processServerCommand, // Control::Rpc_TaskManager,
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
