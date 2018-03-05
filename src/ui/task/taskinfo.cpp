#include "taskinfo.h"

#include <QDataStream>
#include <QMetaEnum>

#include "../util/dateutil.h"
#include "tasktasix.h"
#include "taskupdatechecker.h"
#include "taskuzonline.h"

#define TASK_INFO_VERSION   1

TaskInfo::TaskInfo(TaskType type, QObject *parent) :
    QObject(parent),
    m_enabled(false),
    m_aborted(false),
    m_intervalHours(24),
    m_type(type),
    m_taskWorker(nullptr)
{
}

TaskInfo::~TaskInfo()
{
    abort();
}

void TaskInfo::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
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
    case Tasix:
        return tr("TAS-IX Addresses Downloader");
    case Uzonline:
        return tr("UzOnline Addresses Downloader");
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
    QDataStream stream(&data, QIODevice::WriteOnly);

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
    if (m_taskWorker) return;

    m_aborted = false;

    TaskWorker *taskWorker = createWorker();

    connect(taskWorker, &TaskWorker::finished,
            this, &TaskInfo::handleFinished);

    setTaskWorker(taskWorker);

    taskWorker->run();
}

void TaskInfo::abort()
{
    if (!m_taskWorker || m_aborted)
        return;

    // to avoid recursive call on worker.abort() -> handleFinished(false) -> abort()
    m_aborted = true;

    m_taskWorker->abort();
    m_taskWorker->deleteLater();

    setTaskWorker(nullptr);
}

void TaskInfo::handleFinished(bool success)
{
    if (!m_taskWorker) return;

    setLastRun(DateUtil::now());
    if (success) {
        setLastSuccess(lastRun());
    }

    emit workFinished(success);

    abort();
}

TaskWorker *TaskInfo::createWorker()
{
    switch (m_type) {
    case UpdateChecker:
        return new TaskUpdateChecker(this);
    case Tasix:
        return new TaskTasix(this);
    case Uzonline:
        return new TaskUzonline(this);
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}
