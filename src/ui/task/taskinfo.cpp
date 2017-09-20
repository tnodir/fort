#include "taskinfo.h"

#include <QDataStream>
#include <QMetaEnum>

#define TASK_INFO_VERSION   1

TaskInfo::TaskInfo(TaskType type, QObject *parent) :
    QObject(parent),
    m_enabled(false),
    m_intervalHours(24),
    m_type(type),
    m_lastRun(QDateTime::currentDateTime()),
    m_lastSuccess(QDateTime::currentDateTime())
{
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
        m_intervalHours = intervalHours;
        emit intervalHoursChanged();
    }
}

QString TaskInfo::title() const
{
    switch (m_type) {
    case Tasix:
        return tr("TAS-IX Addresses Downloader");
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

void TaskInfo::setLastSuccess(const QDateTime &lastSuccess)
{
    if (m_lastSuccess != lastSuccess) {
        m_lastSuccess = lastSuccess;
        emit lastSuccessChanged();
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

    if (infoVersion != TASK_INFO_VERSION)
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
