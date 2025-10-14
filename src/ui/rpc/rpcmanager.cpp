#include "rpcmanager.h"

#include <QLoggingCategory>

#include <conf/firewallconf.h>
#include <conf/rule.h>
#include <conf/zone.h>
#include <control/controlmanager.h>
#include <control/controlworker.h>
#include <fortglobal.h>
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
#include <rpc/statconnmanagerrpc.h>
#include <rpc/statmanagerrpc.h>
#include <rpc/taskmanagerrpc.h>
#include <util/osutil.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("rpc");

void showErrorBox(const QString &text)
{
    auto windowManager = Fort::windowManager();

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
    if (settings()->isService()) {
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
    StatConnManagerRpc::setupServerSignals(this);
    TaskManagerRpc::setupServerSignals(this);
}

void RpcManager::setupClient()
{
    auto controlManager = Fort::dependency<ControlManager>();

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
    return client()->waitResult(m_resultCommand);
}

void RpcManager::sendResult(ControlWorker *w, bool ok, const QVariantList &args)
{
    // DBG: qCDebug(LC) << "Send Result to Client: id:" << w->id() << ok << args.size();

    w->sendResult(ok, args);
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
    const auto &clients = controlManager()->clients();
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
    return !settings()->isPasswordRequired() || w->isClientValidated();
}

void RpcManager::initClientOnServer(ControlWorker *w) const
{
    w->setIsServiceClient(true);

    AutoUpdateManagerRpc::processInitClient(w);
    DriverManagerRpc::processInitClient(w);
}

bool RpcManager::processCommandRpc(const ProcessCommandArgs &p, ProcessCommandResult &r)
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
        return processManagerRpc(p, r);
    }
}

static const processCommand_func processManager_funcList[] = {
    &AppInfoManagerRpc::processServerCommand, // Control::Rpc_AppInfoManager,
    &AutoUpdateManagerRpc::processServerCommand, // Control::Rpc_AutoUpdateManager,
    &ConfManagerRpc::processServerCommand, // Control::Rpc_ConfManager,
    &ConfAppManagerRpc::processServerCommand, // Control::Rpc_ConfAppManager,
    &ConfRuleManagerRpc::processServerCommand, // Control::Rpc_ConfRuleManager,
    &ConfZoneManagerRpc::processServerCommand, // Control::Rpc_ConfZoneManager,
    &DriverManagerRpc::processServerCommand, // Control::Rpc_DriverManager,
    &QuotaManagerRpc::processServerCommand, // Control::Rpc_QuotaManager,
    &StatManagerRpc::processServerCommand, // Control::Rpc_StatManager,
    &StatConnManagerRpc::processServerCommand, // Control::Rpc_StatBlockManager,
    &ServiceInfoManagerRpc::processServerCommand, // Control::Rpc_ServiceInfoManager,
    &TaskManagerRpc::processServerCommand, // Control::Rpc_TaskManager,
};

bool RpcManager::processManagerRpc(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    if (commandRequiresValidation(p.command) && !checkClientValidated(p.worker)) {
        r.errorMessage = "Client is not validated";
        r.isSendResult = true;
        return false;
    }

    const Control::RpcManager rpcManager = Control::managerByCommand(p.command);

    if (rpcManager < Control::Rpc_AppInfoManager || rpcManager > Control::Rpc_TaskManager) {
        r.errorMessage = "Unknown command";
        return false;
    }

    const int funcIndex = rpcManager - Control::Rpc_AppInfoManager;
    const processCommand_func func = processManager_funcList[funcIndex];

    return func(p, r);
}
