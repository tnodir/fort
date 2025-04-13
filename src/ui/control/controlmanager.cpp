#include "controlmanager.h"

#include <QApplication>
#include <QHash>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>

#include <fort_version.h>

#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/variantutil.h>

#include "command/controlcommandmanager.h"
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
    socket->setParent(w);
    w->setupForAsync();
    w->setIsServiceClient(true);

    connect(w, &ControlWorker::requestReady, this, &ControlManager::processRequest);

    w->setServerName(getServerName(/*isService=*/true));

    return w;
}

bool ControlManager::listen()
{
    if (m_server) {
        return m_server->isListening();
    }

    const bool isService = IoC<FortSettings>()->isService();

    m_server = new QLocalServer(this);
    m_server->setMaxPendingConnections(maxClientsCount);

    m_server->setSocketOptions(
            isService ? QLocalServer::WorldAccessOption : QLocalServer::NoOptions);

    if (!m_server->listen(getServerName(isService))) {
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

bool ControlManager::processCommandClient(Control::CommandResult &commandResult)
{
    static QHash<QString, Control::Command> g_commandsMap = {
        { "home", Control::CommandHome },
        { "filter", Control::CommandFilter },
        { "filter-mode", Control::CommandFilterMode },
        { "block", Control::CommandBlock },
        { "prog", Control::CommandProg },
        { "conf", Control::CommandConf },
        { "backup", Control::CommandBackup },
        { "zone", Control::CommandZone },
    };

    OsUtil::attachConsole();

    const auto settings = IoC<FortSettings>();

    const Control::Command command =
            g_commandsMap.value(settings->controlCommand(), Control::CommandNone);

    if (command == Control::CommandNone) {
        OsUtil::writeToConsole({ "Unknown control command:", settings->controlCommand() });
        return false;
    }

    const QVariantList args = ControlWorker::buildArgs(settings->args());
    if (args.isEmpty()) {
        OsUtil::writeToConsole({ "Empty arguments for command:", settings->controlCommand() });
        return false;
    }

    OsUtil::allowOtherForegroundWindows(); // let the running instance to activate a window

    return postCommand(command, args, &commandResult);
}

bool ControlManager::postCommand(
        Control::Command command, const QVariantList &args, Control::CommandResult *commandResult)
{
    QLocalSocket socket;
    ControlWorker w(&socket);
    w.setupForAsync();

    Control::Command commandOk;

    connect(&w, &ControlWorker::requestReady, this,
            [&commandOk, commandResult](Control::Command command, const QVariantList &args) {
                commandOk = command;

                if (args.size() < 2)
                    return;

                if (commandResult) {
                    *commandResult = args[0].value<Control::CommandResult>();
                }

                const auto message = args[1].toString();
                if (message.isEmpty())
                    return;

                OsUtil::writeToConsole(QStringList { message });
            });

    // Connect to UI process or Service
    if (!connectToAnyServer(w))
        return false;

    // Send data
    if (!w.postCommand(command, args))
        return false;

    // Read response
    if (!w.waitResult(commandOk))
        return false;

    w.closeForAsync();

    return (commandOk == Control::Rpc_Result_Ok);
}

bool ControlManager::connectToAnyServer(ControlWorker &w)
{
    // Connect to UI process server
    w.setServerName(getServerName(/*isService=*/false));
    if (w.connectToServer())
        return true;

    // Connect to Service server
    w.setServerName(getServerName(/*isService=*/true));
    if (w.connectToServer())
        return true;

    return false;
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
        socket->setParent(w);
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

    qCDebug(LC) << "Client disconnected: id:" << w->id();

    w->deleteLater();
    m_clients.removeOne(w);
}

bool ControlManager::processRequest(Control::Command command, const QVariantList &args)
{
    ControlWorker *w = qobject_cast<ControlWorker *>(sender());
    if (Q_UNLIKELY(!w))
        return false;

    // DBG: qCDebug(LC) << "Client requested: id:" << w->id() << command << args.size();

    // XXX: OsUtil::setThreadIsBusy(true);

    Control::CommandResult commandResult = Control::CommandResultNone;
    QString errorMessage;
    const bool ok = ControlCommandManager::processCommand({
            .worker = w,
            .command = command,
            .commandResult = commandResult,
            .args = args,
            .errorMessage = errorMessage,
    });

    if (!ok) {
        qCWarning(LC) << "Bad command:" << command << args << ':' << errorMessage;
    }

    w->sendResult(ok, { commandResult, errorMessage });

    // XXX: OsUtil::setThreadIsBusy(false);

    return ok;
}

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : OsUtil::userName()) + "Pipe";
}
