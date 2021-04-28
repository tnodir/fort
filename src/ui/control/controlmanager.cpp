#include "controlmanager.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>

#include <fort_version.h>

#include "../conf/appgroup.h"
#include "../conf/firewallconf.h"
#include "../util/fileutil.h"
#include "controlworker.h"
#include "fortmanager.h"
#include "fortsettings.h"

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

bool ControlManager::isClient() const
{
    return !settings()->controlCommand().isEmpty();
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
        command = Control::CommandConf;
    } else if (settings()->controlCommand() == "prog") {
        command = Control::CommandProg;
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

    return worker.sendCommand(command, Control::Rpc_None, 0, args) && worker.waitForSent();
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

        connect(worker, &ControlWorker::requestReady, this, &ControlManager::processRequest);
        connect(worker, &ControlWorker::destroyed, this,
                [&] { m_clients.removeOne(qobject_cast<ControlWorker *>(sender())); });

        m_clients.append(worker);
    }
}

bool ControlManager::processRequest(Control::Command command, const QVariantList &args)
{
    QString errorMessage;
    if (!processCommand(command, args, errorMessage)) {
        logWarning() << "Bad control command" << errorMessage << ':' << command << args;
        return false;
    }
    return true;
}

bool ControlManager::processCommand(
        Control::Command command, const QVariantList &args, QString &errorMessage)
{
    bool ok = false;
    const int argsSize = args.size();

    if (command == Control::CommandConf) {
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

            ok = appGroup->setProperty(groupPropName.toLatin1(), args.at(3));
        } else {
            ok = conf->setProperty(confPropName.toLatin1(), args.at(1));
        }

        if (ok) {
            fortManager()->saveOriginConf(tr("Control command executed"), onlyFlags);
        }
    } else if (command == Control::CommandProg) {
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

            ok = fortManager()->showProgramEditForm(args.at(1).toString());
        }
    }

    if (!ok) {
        errorMessage = "Invalid command";
    }
    return ok;
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
