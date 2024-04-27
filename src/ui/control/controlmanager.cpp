#include "controlmanager.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>

#include <fort_version.h>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/rpcmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "controlworker.h"

namespace {

constexpr int maxClientsCount = 9;

const QLoggingCategory LC("control");

}

ControlManager::ControlManager(QObject *parent) : QObject(parent) { }

ControlManager::~ControlManager()
{
    close();
}

void ControlManager::setUp()
{
    // Process control commands from clients
    listen();
}

bool ControlManager::isCommandClient() const
{
    return !IoC<FortSettings>()->controlCommand().isEmpty();
}

ControlWorker *ControlManager::newServiceClient(QObject *parent) const
{
    auto socket = new QLocalSocket();
    auto w = new ControlWorker(socket, parent);
    w->setupForAsync();
    w->setIsServiceClient(true);

    connect(w, &ControlWorker::requestReady, this, &ControlManager::processRequest);

    w->setServerName(getServerName(/*isService=*/true));

    return w;
}

bool ControlManager::listen()
{
    const auto settings = IoC<FortSettings>();

    if (m_server) {
        return m_server->isListening();
    }

    m_server = new QLocalServer(this);
    m_server->setMaxPendingConnections(maxClientsCount);
    m_server->setSocketOptions(
            settings->isService() ? QLocalServer::WorldAccessOption : QLocalServer::NoOptions);

    if (!m_server->listen(getServerName(settings->isService()))) {
        qCWarning(LC) << "Server listen error:" << m_server->errorString();
        return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, &ControlManager::onNewConnection);

    return true;
}

void ControlManager::close()
{
    if (m_server) {
        m_server->close();
        m_server = nullptr;
    }
}

void ControlManager::closeAllClients()
{
    for (ControlWorker *w : m_clients) {
        w->close();
    }
}

bool ControlManager::processCommandClient()
{
    const auto settings = IoC<FortSettings>();

    Control::Command command;
    if (settings->controlCommand() == "prog") {
        command = Control::Prog;
    } else {
        qCWarning(LC) << "Unknown control command:" << settings->controlCommand();
        return false;
    }

    const QVariantList args = ControlWorker::buildArgs(settings->args());
    if (args.isEmpty())
        return false;

    OsUtil::allowOtherForegroundWindows(); // let the running instance to activate a window

    return postCommand(command, args);
}

bool ControlManager::postCommand(Control::Command command, const QVariantList &args)
{
    QLocalSocket socket;
    ControlWorker w(&socket);

    // Connect to server
    w.setServerName(getServerName(/*isService=*/false));
    if (!w.connectToServer())
        return false;

    // Send data
    if (!w.sendCommand(command, args))
        return false;

    w.waitForSent();

    return true;
}

void ControlManager::onNewConnection()
{
    const bool hasPassword = IoC<FortSettings>()->hasPassword();

    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        if (m_clients.size() > maxClientsCount) {
            qCWarning(LC) << "Client dropped: Count limit";
            delete socket;
            continue;
        }

        auto w = new ControlWorker(socket, this);
        w->setupForAsync();

        w->setIsClientValidated(!hasPassword);

        connect(w, &ControlWorker::disconnected, this, &ControlManager::onDisconnected);
        connect(w, &ControlWorker::requestReady, this, &ControlManager::processRequest);

        m_clients.append(w);

        qCDebug(LC) << "Client connected: id:" << w->id() << "count:" << m_clients.size();
    }
}

void ControlManager::onDisconnected()
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (Q_UNLIKELY(!w))
        return;

    w->deleteLater();
    m_clients.removeOne(w);

    qCDebug(LC) << "Client disconnected: id:" << w->id();
}

bool ControlManager::processRequest(Control::Command command, const QVariantList &args)
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (Q_UNLIKELY(!w))
        return false;

    // DBG: qCDebug(LC) << "Client requested: id:" << w->id() << command << args.size();

    // XXX: OsUtil::setThreadIsBusy(true);

    QString errorMessage;
    const bool success = processCommand({
            .worker = w,
            .command = command,
            .args = args,
            .errorMessage = errorMessage,
    });

    if (!success) {
        qCWarning(LC) << "Bad command" << errorMessage << ':' << command << args;
    }

    // XXX: OsUtil::setThreadIsBusy(false);

    return success;
}

bool ControlManager::processCommand(const ProcessCommandArgs &p)
{
    switch (p.command) {
    case Control::Prog:
        if (processCommandProg(p))
            return true;
        break;
    default:
        if (IoC<RpcManager>()->processCommandRpc(p))
            return true;
    }

    if (p.errorMessage.isEmpty()) {
        p.errorMessage = "Invalid command";
    }
    return false;
}

bool ControlManager::processCommandProg(const ProcessCommandArgs &p)
{
    const ProgAction progAction = progActionByText(p.args.value(0).toString());
    if (progAction == ProgActionNone) {
        p.errorMessage = "prog add|del|allow|block|kill|show [<app-path>]";
        return false;
    }

    if (!checkProgActionPassword(progAction)) {
        p.errorMessage = "Password required";
        return false;
    }

    const QString appPath = p.args.value(1).toString();

    return processCommandProgAction(progAction, appPath);
}

bool ControlManager::processCommandProgAction(ProgAction progAction, const QString &appPath)
{
    switch (progAction) {
    case ProgActionAdd: {
        return IoC<WindowManager>()->showProgramEditForm(appPath);
    }
    case ProgActionDel: {
        return IoC<ConfAppManager>()->deleteAppPath(appPath);
    }
    case ProgActionAllow:
    case ProgActionBlock:
    case ProgActionKill: {
        const bool blocked = (progAction != ProgActionAllow);
        const bool killProcess = (progAction == ProgActionKill);

        return IoC<ConfAppManager>()->addOrUpdateAppPath(appPath, blocked, killProcess);
    }
    case ProgActionShow: {
        IoC<WindowManager>()->exposeHomeWindow();
        return true;
    }
    default:
        return false;
    }
}

bool ControlManager::checkProgActionPassword(ProgAction progAction)
{
    constexpr const quint32 passwordRequiredActions =
            ProgActionDel | ProgActionAllow | ProgActionBlock | ProgActionKill;

    return (passwordRequiredActions & progAction) == 0
            || IoC<WindowManager>()->checkPassword(/*temporary=*/true);
}

ControlManager::ProgAction ControlManager::progActionByText(const QString &text)
{
    if (text == "add")
        return ProgActionAdd;

    if (text == "del")
        return ProgActionDel;

    if (text == "allow")
        return ProgActionAllow;

    if (text == "block")
        return ProgActionBlock;

    if (text == "kill")
        return ProgActionKill;

    if (text == "show")
        return ProgActionShow;

    return ProgActionNone;
}

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : OsUtil::userName()) + "Pipe";
}
