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
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "controlworker.h"

namespace {

constexpr int maxClientsCount = 9;

const QLoggingCategory LC("control");

bool checkActionPassword(quint32 action, quint32 passwordNotRequiredActions = 0)
{
    if ((action & passwordNotRequiredActions) != 0)
        return true;

    return IoC<WindowManager>()->checkPassword(/*temporary=*/true);
}

bool processCommandHome(const ProcessCommandArgs &p)
{
    const auto commandText = p.args.value(0).toString();

    if (commandText == "show") {
        return IoC<WindowManager>()->exposeHomeWindow();
    }

    p.errorMessage = "Usage: home show";
    return false;
}

enum BlockAction : qint8 {
    BlockActionInvalid = -1,
    BlockActionNone = 0,
    BlockActionAll,
    BlockActionInet,
};

bool processCommandProgBlock(BlockAction blockAction)
{
    auto confManager = IoC<ConfManager>();

    auto conf = confManager->conf();
    conf->setBlockTrafficIndex(blockAction);

    return confManager->saveFlags();
}

BlockAction blockActionByText(const QString &commandText)
{
    if (commandText == "none")
        return BlockActionNone;

    if (commandText == "all")
        return BlockActionAll;

    if (commandText == "internet")
        return BlockActionInet;

    return BlockActionInvalid;
}

bool processCommandBlock(const ProcessCommandArgs &p)
{
    const BlockAction blockAction = blockActionByText(p.args.value(0).toString());
    if (blockAction == BlockActionInvalid) {
        p.errorMessage = "Usage: block none|all|internet";
        return false;
    }

    if (!checkActionPassword(blockAction)) {
        p.errorMessage = "Password required";
        return false;
    }

    return processCommandProgBlock(blockAction);
}

enum ProgAction : quint32 {
    ProgActionNone = 0,
    ProgActionAdd = (1 << 0),
    ProgActionDel = (1 << 1),
    ProgActionAllow = (1 << 2),
    ProgActionBlock = (1 << 3),
    ProgActionKill = (1 << 4),
};

bool processCommandProgAction(ProgAction progAction, const QString &appPath)
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
    default:
        return false;
    }
}

ProgAction progActionByText(const QString &commandText)
{
    if (commandText == "add")
        return ProgActionAdd;

    if (commandText == "del")
        return ProgActionDel;

    if (commandText == "allow")
        return ProgActionAllow;

    if (commandText == "block")
        return ProgActionBlock;

    if (commandText == "kill")
        return ProgActionKill;

    return ProgActionNone;
}

bool processCommandProg(const ProcessCommandArgs &p)
{
    const ProgAction progAction = progActionByText(p.args.value(0).toString());
    if (progAction == ProgActionNone) {
        p.errorMessage = "Usage: prog add|del|allow|block|kill|show <app-path>";
        return false;
    }

    if (!checkActionPassword(progAction, ProgActionAdd)) {
        p.errorMessage = "Password required";
        return false;
    }

    const QString appPath = p.args.value(1).toString();

    return processCommandProgAction(progAction, appPath);
}

enum BackupAction : quint32 {
    BackupActionNone = 0,
    BackupActionExport = (1 << 0),
    BackupActionImport = (1 << 1),
};

bool processCommandBackupAction(BackupAction backupAction, const QString &dirPath)
{
    switch (backupAction) {
    case BackupActionExport: {
        return IoC<ConfManager>()->exportBackup(dirPath);
    }
    case BackupActionImport: {
        return IoC<ConfManager>()->importBackup(dirPath);
    }
    default:
        return false;
    }
}

BackupAction backupActionByText(const QString &commandText)
{
    if (commandText == "export")
        return BackupActionExport;

    if (commandText == "import")
        return BackupActionImport;

    return BackupActionNone;
}

bool processCommandBackup(const ProcessCommandArgs &p)
{
    const BackupAction backupAction = backupActionByText(p.args.value(0).toString());
    if (backupAction == BackupActionNone) {
        p.errorMessage = "Usage: backup export/import <dir-path>";
        return false;
    }

    if (!checkActionPassword(backupAction)) {
        p.errorMessage = "Password required";
        return false;
    }

    const QString dirPath = p.args.value(1).toString();

    return processCommandBackupAction(backupAction, dirPath);
}

enum ZoneAction : quint32 {
    ZoneActionNone = 0,
    ZoneActionUpdate = (1 << 0),
};

bool processCommandZoneAction(ZoneAction zoneAction)
{
    switch (zoneAction) {
    case ZoneActionUpdate: {
        IoC<TaskManager>()->runTask(TaskInfo::ZoneDownloader);
        return true;
    }
    default:
        return false;
    }
}

ZoneAction zoneActionByText(const QString &commandText)
{
    if (commandText == "update")
        return ZoneActionUpdate;

    return ZoneActionNone;
}

bool processCommandZone(const ProcessCommandArgs &p)
{
    const ZoneAction zoneAction = zoneActionByText(p.args.value(0).toString());
    if (zoneAction == ZoneActionNone) {
        p.errorMessage = "Usage: zone update";
        return false;
    }

    if (!checkActionPassword(zoneAction)) {
        p.errorMessage = "Password required";
        return false;
    }

    return processCommandZoneAction(zoneAction);
}

bool processCommand(const ProcessCommandArgs &p)
{
    bool ok;

    switch (p.command) {
    case Control::CommandHome: {
        ok = processCommandHome(p);
    } break;
    case Control::CommandBlock: {
        ok = processCommandBlock(p);
    } break;
    case Control::CommandProg: {
        ok = processCommandProg(p);
    } break;
    case Control::CommandBackup: {
        ok = processCommandBackup(p);
    } break;
    case Control::CommandZone: {
        ok = processCommandZone(p);
    } break;
    default:
        ok = IoC<RpcManager>()->processCommandRpc(p);
    }

    if (!ok && p.errorMessage.isEmpty()) {
        p.errorMessage = "Invalid command";
    }

    return ok;
}

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

bool ControlManager::processCommandClient()
{
    const auto settings = IoC<FortSettings>();

    Control::Command command;
    if (settings->controlCommand() == "home") {
        command = Control::CommandHome;
    } else if (settings->controlCommand() == "block") {
        command = Control::CommandBlock;
    } else if (settings->controlCommand() == "prog") {
        command = Control::CommandProg;
    } else if (settings->controlCommand() == "backup") {
        command = Control::CommandBackup;
    } else if (settings->controlCommand() == "zone") {
        command = Control::CommandZone;
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

    // Connect to UI process or Service
    if (!connectToAnyServer(w))
        return false;

    // Send data
    if (!w.sendCommand(command, args))
        return false;

    w.waitForSent();

    return true;
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

QString ControlManager::getServerName(bool isService)
{
    return QLatin1String(APP_BASE) + (isService ? "Svc" : OsUtil::userName()) + "Pipe";
}
