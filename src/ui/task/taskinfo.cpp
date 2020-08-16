#include "taskinfo.h"

#include <QDataStream>
#include <QMetaEnum>

#include "../util/dateutil.h"
#include "taskmanager.h"
#include "taskupdatechecker.h"
#include "taskzonedownloader.h"

#define TASK_INFO_VERSION   1

TaskInfo::TaskInfo(TaskType type, TaskManager &taskManager) :
    QObject(&taskManager),
    m_enabled(false),
    m_running(false),
    m_aborted(false),
    m_type(type)
{
}

TaskInfo::~TaskInfo()
{
    abort();
}

TaskManager *TaskInfo::taskManager() const
{
    return qobject_cast<TaskManager *>(parent());
}

FortManager *TaskInfo::fortManager() const
{
    return taskManager()->fortManager();
}

void TaskInfo::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

void TaskInfo::setRunning(bool running)
{
    if (m_running != running) {
        m_running = running;
        emit enabledChanged();
    }
}

void TaskInfo::setIntervalHours(int intervalHours)
{
    if (m_intervalHours != intervalHours) {
        m_intervalHours = quint16(intervalHours);
        emit intervalHoursChanged();
    }
}

QString TaskInfo::title() const
{
    switch (m_type) {
    case UpdateChecker:
        return tr("Update Checker");
    case ZoneDownloader:
        return tr("Zones Downloader");
    default:
        Q_UNREACHABLE();
        return QString();
    }
}

void TaskInfo::setType(TaskInfo::TaskType type)
{
    if (m_type != type) {
        m_type = type;
        emit typeChanged();
    }
}

void TaskInfo::setLastRun(const QDateTime &lastRun)
{
    if (m_lastRun != lastRun) {
        m_lastRun = lastRun;
        emit lastRunChanged();
    }
}

QDateTime TaskInfo::plannedRun() const
{
    return m_lastRun.addSecs(m_intervalHours * 60 * 60);
}

void TaskInfo::setLastSuccess(const QDateTime &lastSuccess)
{
    if (m_lastSuccess != lastSuccess) {
        m_lastSuccess = lastSuccess;
        emit lastSuccessChanged();
    }
}

void TaskInfo::setTaskWorker(TaskWorker *taskWorker)
{
    if (m_taskWorker != taskWorker) {
        m_taskWorker = taskWorker;
        emit taskWorkerChanged();
    }
}

void TaskInfo::rawData(QByteArray &data) const
{
    QDataStream stream(&data, QDataStream::WriteOnly);

    // Store data
    const quint16 infoVersion = TASK_INFO_VERSION;
    const quint8 enabled = m_enabled;
    const quint16 intervalHours = m_intervalHours;

    stream
            << infoVersion
            << enabled
            << intervalHours
            << m_lastRun
            << m_lastSuccess;
}

void TaskInfo::setRawData(const QByteArray &data)
{
    QDataStream stream(data);

    // Check version
    quint16 infoVersion;
    stream >> infoVersion;

    if (infoVersion > TASK_INFO_VERSION)
        return;

    // Load data
    quint8 enabled;
    quint16 intervalHours;

    stream
            >> enabled
            >> intervalHours
            >> m_lastRun
            >> m_lastSuccess;

    m_enabled = enabled;
    m_intervalHours = intervalHours;
}

QString TaskInfo::typeToString(TaskInfo::TaskType type)
{
    const QMetaEnum typeEnum = QMetaEnum::fromType<TaskType>();
    return QString::fromLatin1(typeEnum.valueToKey(type));
}

TaskInfo::TaskType TaskInfo::stringToType(const QString &name)
{
    const QMetaEnum typeEnum = QMetaEnum::fromType<TaskType>();
    return static_cast<TaskInfo::TaskType>(
                typeEnum.keyToValue(name.toLatin1()));
}

void TaskInfo::run()
{
    if (taskWorker()) return;

    setRunning(true);
    emit workStarted();

    setupTaskWorker();
    runTaskWorker();
}

void TaskInfo::setupTaskWorker()
{
    TaskWorker *taskWorker = createWorker();

    connect(taskWorker, &TaskWorker::finished,
            this, &TaskInfo::handleFinished);

    setTaskWorker(taskWorker);

    m_aborted = false;
}

void TaskInfo::runTaskWorker()
{
    if (aborted() || taskWorker() == nullptr)
        return;

    taskWorker()->run();
}

void TaskInfo::abort()
{
    if (aborted())
        return;

    // to avoid recursive call on worker.abort() -> handleFinished(false) -> abort()
    m_aborted = true;

    if (taskWorker() != nullptr) {
        taskWorker()->abort();
        taskWorker()->deleteLater();

        setTaskWorker(nullptr);
    }
}

void TaskInfo::handleFinished(bool success)
{
    if (!running())
        return;

    setLastRun(DateUtil::now());
    if (success) {
        setLastSuccess(lastRun());
    }

    setRunning(false);
    emit workFinished(success);

    abort();
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
