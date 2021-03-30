#include "controlmanager.h"

#include <QLoggingCategory>
#include <QThreadPool>

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

ControlManager::ControlManager(const QString &globalName, const QString &command, QObject *parent) :
    QObject(parent),
    m_isClient(!command.isEmpty()),
    m_command(command),
    m_semaphore(globalName + QLatin1String("_ControlSemaphore"), 0,
            isClient() ? QSystemSemaphore::Open : QSystemSemaphore::Create),
    m_sharedMemory(globalName + QLatin1String("_ControlSharedMemory"))
{
}

ControlManager::~ControlManager()
{
    abort();
}

bool ControlManager::listen(FortManager *fortManager)
{
    if (m_sharedMemory.size() > 0)
        return true;

    if (!m_sharedMemory.create(4096)) {
        logWarning() << "Shared Memory create error:" << m_sharedMemory.errorString();
        return false;
    }

    m_fortManager = fortManager;

    if (!m_worker) {
        setupWorker();
    }

    return true;
}

bool ControlManager::post(const QStringList &args)
{
    if (!m_sharedMemory.attach()) {
        logWarning() << "Shared Memory attach error:" << m_sharedMemory.errorString();
        return false;
    }

    ControlWorker worker(&m_semaphore, &m_sharedMemory);

    return worker.post(m_command, args);
}

void ControlManager::processRequest(const QString &command, const QStringList &args)
{
    QString errorMessage;
    if (!processCommand(command, args, errorMessage)) {
        logWarning() << "Bad control command" << errorMessage << ':' << command << args;
    }
}

bool ControlManager::processCommand(
        const QString &command, const QStringList &args, QString &errorMessage)
{
    bool ok = false;
    const int argsSize = args.size();

    if (command == "conf") {
        if (argsSize < 2) {
            errorMessage = "conf <property> <value>";
            return false;
        }

        auto conf = m_fortManager->conf();
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
            m_fortManager->saveOriginConf(tr("Control command executed"), onlyFlags);
        }
    } else if (command == "prog") {
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

            ok = m_fortManager->showProgramEditForm(args.at(1));
        }
    }

    if (!ok) {
        errorMessage = "Invalid command";
    }
    return ok;
}

void ControlManager::setupWorker()
{
    m_worker = new ControlWorker(&m_semaphore, &m_sharedMemory, this);
    m_worker->setAutoDelete(false);

    connect(m_worker, &ControlWorker::requestReady, this, &ControlManager::processRequest);

    QThreadPool::globalInstance()->start(m_worker);
}

void ControlManager::abort()
{
    if (!m_worker)
        return;

    m_worker->disconnect(this);

    m_worker->abort();
    m_worker->deleteLater();
    m_worker = nullptr;
}
