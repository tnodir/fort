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
    const bool ok = worker.postCommand(command, settings()->args());

    return ok;
}

void ControlManager::onNewConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        auto worker = new ControlWorker(socket, this);
        worker->setupForAsync();

        connect(worker, &ControlWorker::requestReady, this, &ControlManager::processRequest);
    }
}

bool ControlManager::processRequest(Control::Command command, const QStringList &args)
{
    QString errorMessage;
    if (!processCommand(command, args, errorMessage)) {
        logWarning() << "Bad control command" << errorMessage << ':' << command << args;
        return false;
    }
    return true;
}

bool ControlManager::processCommand(
        Control::Command command, const QStringList &args, QString &errorMessage)
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

        const auto confPropName = args.at(0);

        if (confPropName == "appGroup") {
            if (argsSize < 4) {
                errorMessage = "conf appGroup <group-name> <property> <value>";
                return false;
            }

            auto appGroup = conf->appGroupByName(args.at(1));
            const auto groupPropName = args.at(2);
            onlyFlags = (groupPropName == "enabled");

            ok = appGroup->setProperty(groupPropName.toLatin1(), QVariant(args.at(3)));
        } else {
            ok = conf->setProperty(confPropName.toLatin1(), QVariant(args.at(1)));
        }

        if (ok) {
            fortManager()->saveOriginConf(tr("Control command executed"), onlyFlags);
        }
    } else if (command == Control::CommandProg) {
        if (argsSize < 1) {
            errorMessage = "prog <command>";
            return false;
        }

        const auto progCommand = args.at(0);

        if (progCommand == "add") {
            if (argsSize < 2) {
                errorMessage = "prog add <app-path>";
                return false;
            }

            ok = fortManager()->showProgramEditForm(args.at(1));
        }
    }

    if (!ok) {
        errorMessage = "Invalid command";
    }
    return ok;
}

void ControlManager::abort()
{
    m_server->close();
}

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : QString()) + "Pipe";
}
