#include "taskinfo.h"

#include <QMetaEnum>

#include <util/dateutil.h>

#include "taskeditinfo.h"
#include "taskmanager.h"
#include "taskupdatechecker.h"
#include "taskzonedownloader.h"

TaskInfo::TaskInfo(TaskType type, TaskManager &taskManager) :
    QObject(&taskManager), m_type(type) { }

TaskInfo::~TaskInfo()
{
    abortTask();
}

TaskManager *TaskInfo::taskManager() const
{
    return qobject_cast<TaskManager *>(parent());
}

QString TaskInfo::title() const
{
    return title(m_type);
}

QString TaskInfo::title(TaskType type)
{
    switch (type) {
    case UpdateChecker:
        return tr("Fort Firewall Update Checker");
    case ZoneDownloader:
        return tr("Zones Downloader");
    case AppPurger:
        return tr("Purge Obsolete Programs");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

void TaskInfo::editFromVariant(const QVariant &v)
{
    const TaskEditInfo task(v.toULongLong());

    setEnabled(task.enabled());
    setRunOnStartup(task.runOnStartup());
    setDelayStartup(task.delayStartup());
    setMaxRetries(task.maxRetries());
    setRetrySeconds(task.retrySeconds());
    setIntervalHours(task.intervalHours());
}

qint64 TaskInfo::secondsToRun(const QDateTime &now, bool isFirstRun)
{
    if (isFirstRun && runOnStartup()) {
        if (delayStartup()) {
            m_plannedRun = now.addSecs(retrySeconds());
            return retrySeconds();
        }
        return 0;
    }

    QDateTime plannedRun;

    if (!m_plannedRun.isNull()) {
        plannedRun = m_plannedRun;
    } else {
        const qint64 delaySecs = (m_failedCount > 0) ? retrySeconds() : (intervalHours() * 60 * 60);

        plannedRun = lastRun().addSecs(delaySecs);
    }

    return now.secsTo(plannedRun);
}

QString TaskInfo::typeToString(TaskInfo::TaskType type)
{
    const QMetaEnum typeEnum = QMetaEnum::fromType<TaskType>();
    return QString::fromLatin1(typeEnum.valueToKey(type));
}

TaskInfo::TaskType TaskInfo::stringToType(const QString &name)
{
    const QMetaEnum typeEnum = QMetaEnum::fromType<TaskType>();
    return static_cast<TaskInfo::TaskType>(typeEnum.keyToValue(name.toLatin1()));
}

void TaskInfo::run()
{
    if (taskWorker())
        return;

    setRunning(true);
    emit workStarted();

    setupTaskWorker();
    runTaskWorker();
}

void TaskInfo::setupTaskWorker()
{
    TaskWorker *taskWorker = createWorker();

    connect(taskWorker, &TaskWorker::finished, this, &TaskInfo::handleFinished);

    setTaskWorker(taskWorker);

    m_aborted = false;
}

void TaskInfo::runTaskWorker()
{
    if (taskWorker()) {
        taskWorker()->run();
    }
}

void TaskInfo::abortTask()
{
    if (aborted())
        return;

    // to avoid recursive call on worker.abort() -> handleFinished(false) -> abort()
    m_aborted = true;

    if (taskWorker()) {
        taskWorker()->finish();
        taskWorker()->deleteLater();

        setTaskWorker(nullptr);
    }
}

void TaskInfo::handleFinished(bool success)
{
    if (!running())
        return;

    setLastRun(DateUtil::now());

    m_plannedRun = {}; // clear planned run

    if (success) {
        setLastSuccess(lastRun());

        m_failedCount = 0;
    } else {
        if (++m_failedCount > maxRetries()) {
            m_failedCount = 0;
        }
    }

    setRunning(false);
    emit workFinished(success);

    abortTask();
}

TaskWorker *TaskInfo::createWorker()
{
    switch (m_type) {
    case UpdateChecker:
        return new TaskUpdateChecker(this);
    case ZoneDownloader:
        return new TaskZoneDownloader(this);
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}
