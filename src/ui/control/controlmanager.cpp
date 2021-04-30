#include "controlmanager.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>

#include <fort_version.h>

#include "../conf/appgroup.h"
#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/rpcmanager.h"
#include "../util/fileutil.h"
#include "controlworker.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONTROL_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONTROL_MANAGER, "control")

#define logDebug()    qCDebug(CLOG_CONTROL_MANAGER, )
#define logWarning()  qCWarning(CLOG_CONTROL_MANAGER, )
#define logCritical() qCCritical(CLOG_CONTROL_MANAGER, )

ControlManager::ControlManager(FortSettings *settings, QObject *parent) :
    QObject(parent), m_settings(settings)
{
}

ControlManager::~ControlManager()
{
    abort();
}

RpcManager *ControlManager::rpcManager() const
{
    return fortManager()->rpcManager();
}

bool ControlManager::isCommandClient() const
{
    return !settings()->controlCommand().isEmpty();
}

ControlWorker *ControlManager::newServiceClient(QObject *parent) const
{
    auto socket = new QLocalSocket();
    auto w = new ControlWorker(socket, parent);
    w->setupForAsync();
    w->setIsServiceClient(true);
    socket->connectToServer(getServerName(true));
    return w;
}

bool ControlManager::listen(FortManager *fortManager)
{
    m_fortManager = fortManager;

    Q_ASSERT(!m_server);
    m_server = new QLocalServer(this);
    m_server->setMaxPendingConnections(3);

    if (!m_server->listen(getServerName(settings()->isService()))) {
        logWarning() << "Local Server create error:" << m_server->errorString();
        return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, &ControlManager::onNewConnection);

    return true;
}

bool ControlManager::postCommand()
{
    Control::Command command;
    if (settings()->controlCommand() == "conf") {
        command = Control::Conf;
    } else if (settings()->controlCommand() == "prog") {
        command = Control::Prog;
    } else {
        logWarning() << "Unknown control command:" << settings()->controlCommand();
        return false;
    }

    const QString serverName =
            getServerName(!settings()->isWindowControl() && settings()->hasService());

    QLocalSocket socket;

    // Connect to server
    socket.connectToServer(serverName);
    if (!socket.waitForConnected(1000)) {
        logWarning() << "Connect to server error:" << socket.errorString();
        return false;
    }

    // Send data
    ControlWorker worker(&socket);

    const QVariantList args = ControlWorker::buildArgs(settings()->args());
    if (args.isEmpty())
        return false;

    return worker.sendCommand(command, args) && worker.waitForSent();
}

void ControlManager::onNewConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        constexpr int maxClientsCount = 9;
        if (m_clients.size() > maxClientsCount) {
            logDebug() << "Client dropped";
            delete socket;
            continue;
        }

        auto worker = new ControlWorker(socket, this);
        worker->setupForAsync();

        connect(socket, &QLocalSocket::disconnected, worker, &QObject::deleteLater);
        connect(worker, &ControlWorker::destroyed, this,
                [&] { m_clients.removeOne(qobject_cast<ControlWorker *>(sender())); });
        connect(worker, &ControlWorker::requestReady, this, &ControlManager::processRequest);

        m_clients.append(worker);
    }
}

bool ControlManager::processRequest(Control::Command command, const QVariantList &args)
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (!w)
        return false;

    QString errorMessage;
    if (!processCommand(w, command, args, errorMessage)) {
        logWarning() << "Bad control command" << errorMessage << ':' << command << args;
        return false;
    }
    return true;
}

bool ControlManager::processCommand(
        ControlWorker *w, Control::Command command, const QVariantList &args, QString &errorMessage)
{
    switch (command) {
    case Control::Conf:
        if (processCommandConf(args, errorMessage))
            return true;
        break;
    case Control::Prog:
        if (processCommandProg(args, errorMessage))
            return true;
        break;
    default:
        if (rpcManager()->processCommandRpc(w, command, args, errorMessage))
            return true;
    }

    if (errorMessage.isEmpty()) {
        errorMessage = "Invalid command";
    }
    return false;
}

bool ControlManager::processCommandConf(const QVariantList &args, QString &errorMessage)
{
    const int argsSize = args.size();
    if (argsSize < 2) {
        errorMessage = "conf <property> <value>";
        return false;
    }

    auto conf = fortManager()->conf();
    bool onlyFlags = true;

    const QString confPropName = args.at(0).toString();

    if (confPropName == "appGroup") {
        if (argsSize < 4) {
            errorMessage = "conf appGroup <group-name> <property> <value>";
            return false;
        }

        AppGroup *appGroup = conf->appGroupByName(args.at(1).toString());
        const QString groupPropName = args.at(2).toString();
        onlyFlags = (groupPropName == "enabled");

        if (!appGroup->setProperty(groupPropName.toLatin1(), args.at(3))) {
            errorMessage = "Bad appGroup property";
            return false;
        }
    } else {
        if (!conf->setProperty(confPropName.toLatin1(), args.at(1))) {
            errorMessage = "Bad conf property";
            return false;
        }
    }

    return fortManager()->saveOriginConf(tr("Control command executed"), onlyFlags);
}

bool ControlManager::processCommandProg(const QVariantList &args, QString &errorMessage)
{
    const int argsSize = args.size();
    if (argsSize < 1) {
        errorMessage = "prog <command>";
        return false;
    }

    const QString progCommand = args.at(0).toString();

    if (progCommand == "add") {
        if (argsSize < 2) {
            errorMessage = "prog add <app-path>";
            return false;
        }

        if (!fortManager()->showProgramEditForm(args.at(1).toString())) {
            errorMessage = "Edit Program is already opened";
            return false;
        }

        return true;
    }

    return false;
}

void ControlManager::abort()
{
    if (m_server) {
        m_server->close();
    }
}

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : QString()) + "Pipe";
}
