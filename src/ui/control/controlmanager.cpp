#include "controlmanager.h"

#include <QJSEngine>
#include <QLoggingCategory>
#include <QThreadPool>

#include "../conf/firewallconf.h"
#include "../util/fileutil.h"
#include "controlworker.h"
#include "fortmanager.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONTROL_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONTROL_MANAGER, "fort.controlManager")

ControlManager::ControlManager(const QString &globalName,
                               const QString &scriptPath,
                               QObject *parent) :
    QObject(parent),
    m_isClient(!scriptPath.isEmpty()),
    m_scriptPath(scriptPath),
    m_fortManager(nullptr),
    m_worker(nullptr),
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
        qWarning(CLOG_CONTROL_MANAGER()) << "Shared Memory create error:"
                                         << m_sharedMemory.errorString();
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
        qWarning(CLOG_CONTROL_MANAGER()) << "Shared Memory attach error:"
                                         << m_sharedMemory.errorString();
        return false;
    }

    ControlWorker worker(&m_semaphore, &m_sharedMemory);

    return worker.post(m_scriptPath, args);
}

void ControlManager::processRequest(const QString &scriptPath,
                                    const QStringList &args)
{
    const QString script = FileUtil::readFile(scriptPath);
    if (script.isEmpty()) {
        qWarning(CLOG_CONTROL_MANAGER()) << "Script is empty:"
                                         << scriptPath;
        return;
    }

    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    QJSValue globalObject = engine.globalObject();

    // Arguments
    QJSValue argsJs = engine.newArray(args.size());
    QJSValue argsMapJs = engine.newObject();
    for (int i = 0, n = args.size(); i < n; ++i) {
        const QString &arg = args.at(i);
        argsJs.setProperty(i, arg);

        const int sepPos = arg.indexOf('=');
        if (sepPos > 0) {
            const QString k = arg.left(sepPos);
            const QString v = arg.mid(sepPos + 1);
            argsMapJs.setProperty(k, v);
        }
    }
    globalObject.setProperty("args", argsJs);
    globalObject.setProperty("arg", argsMapJs);

    // FirewallConf
    QJSValue firewallConfJs = engine.newQObject(
                m_fortManager->firewallConf());
    globalObject.setProperty("conf", firewallConfJs);

    // Run the script
    const QJSValue res = engine.evaluate(script, scriptPath);
    if (res.isError()) {
        qWarning(CLOG_CONTROL_MANAGER()) << "Script error:"
                                         << scriptPath << "line"
                                         << res.property("lineNumber").toInt()
                                         << ":" << res.toString();
        return;
    }

    m_fortManager->saveOriginConf(tr("Control script executed"));
}

void ControlManager::setupWorker()
{
    m_worker = new ControlWorker(&m_semaphore, &m_sharedMemory, this);
    m_worker->setAutoDelete(false);

    connect(m_worker, &ControlWorker::requestReady,
            this, &ControlManager::processRequest);

    QThreadPool::globalInstance()->start(m_worker);
}

void ControlManager::abort()
{
    if (!m_worker) return;

    m_worker->disconnect(this);

    m_worker->abort();
    m_worker->deleteLater();
    m_worker = nullptr;
}
