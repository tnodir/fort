#include "controlmanager.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>

#include <fort_version.h>

#include <conf/appgroup.h>
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

    if (!w->connectToServer(getServerName(true))) {
        qCWarning(LC) << "Server connect error:" << socket->state() << socket->errorString();
    }
    return w;
}

bool ControlManager::listen()
{
    const auto settings = IoC<FortSettings>();

    Q_ASSERT(!m_server);
    m_server = new QLocalServer(this);
    m_server->setMaxPendingConnections(3);
    m_server->setSocketOptions(
            settings->isService() ? QLocalServer::WorldAccessOption : QLocalServer::NoOptions);

    if (!m_server->listen(getServerName(settings->isService()))) {
        qCWarning(LC) << "Server listen error:" << m_server->errorString();
        return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, &ControlManager::onNewConnection);

    return true;
}

bool ControlManager::postCommand()
{
    const auto settings = IoC<FortSettings>();

    Control::Command command;
    if (settings->controlCommand() == "prog") {
        command = Control::Prog;
    } else {
        qCWarning(LC) << "Unknown control command:" << settings->controlCommand();
        return false;
    }

    QLocalSocket socket;
    ControlWorker w(&socket);

    // Connect to server
    if (!w.connectToServer(getServerName())) {
        qCWarning(LC) << "Connect to server error:" << socket.errorString();
        return false;
    }

    // Send data
    const QVariantList args = ControlWorker::buildArgs(settings->args());
    if (args.isEmpty())
        return false;

    return w.sendCommand(command, args) && w.waitForSent();
}

void ControlManager::onNewConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        constexpr int maxClientsCount = 9;
        if (m_clients.size() > maxClientsCount) {
            qCDebug(LC) << "Client dropped";
            delete socket;
            continue;
        }

        auto w = new ControlWorker(socket, this);
        w->setupForAsync();

        connect(w, &ControlWorker::disconnected, this, &ControlManager::onDisconnected);
        connect(w, &ControlWorker::requestReady, this, &ControlManager::processRequest);

        m_clients.append(w);

        qCDebug(LC) << "Client connected:" << w->id();
    }
}

void ControlManager::onDisconnected()
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (Q_UNLIKELY(!w))
        return;

    w->deleteLater();
    m_clients.removeOne(w);

    qCDebug(LC) << "Client disconnected:" << w->id();
}

bool ControlManager::processRequest(Control::Command command, const QVariantList &args)
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (Q_UNLIKELY(!w))
        return false;

    QString errorMessage;
    if (!processCommand({ w, command, args, errorMessage })) {
        qCWarning(LC) << "Bad command" << errorMessage << ':' << command << args;
        return false;
    }
    return true;
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
    const int argsSize = p.args.size();
    if (argsSize < 1) {
        p.errorMessage = "prog <command>";
        return false;
    }

    const QString progCommand = p.args.at(0).toString();

    if (progCommand == "add") {
        if (argsSize < 2) {
            p.errorMessage = "prog add <app-path>";
            return false;
        }

        if (!IoC<WindowManager>()->showProgramEditForm(p.args.at(1).toString())) {
            p.errorMessage = "Edit Program is already opened";
            return false;
        }

        return true;
    }

    return false;
}

void ControlManager::close()
{
    if (m_server) {
        m_server->close();
    }
}

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : OsUtil::userName()) + "Pipe";
}
