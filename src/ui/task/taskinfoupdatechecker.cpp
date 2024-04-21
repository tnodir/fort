#include "taskinfoupdatechecker.h"

#include <QDataStream>
#include <QLoggingCategory>
#include <QVersionNumber>

#include <fort_version.h>

#include "taskmanager.h"
#include "taskupdatechecker.h"

namespace {

const QLoggingCategory LC("task.updateChecker");

constexpr int TASK_INFO_VERSION = 3;

}

TaskInfoUpdateChecker::TaskInfoUpdateChecker(TaskManager &taskManager) :
    TaskInfo(UpdateChecker, taskManager)
{
}

bool TaskInfoUpdateChecker::isNewVersion() const
{
    return !version().isEmpty() && version() != APP_VERSION_STR
            && QVersionNumber::fromString(version())
            > QVersionNumber(APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
}

QByteArray TaskInfoUpdateChecker::data() const
{
    QByteArray data;
    QDataStream stream(&data, QDataStream::WriteOnly);

    // Store data
    const quint16 infoVersion = TASK_INFO_VERSION;

    stream << infoVersion << QString::fromLatin1(APP_VERSION_STR) << m_version << m_releaseText
           << m_downloadUrl << m_downloadSize;

    return data;
}

void TaskInfoUpdateChecker::setData(const QByteArray &data)
{
    QDataStream stream(data);

    // Check version
    quint16 infoVersion;
    stream >> infoVersion;

    if (infoVersion > TASK_INFO_VERSION)
        return;

    // COMPAT: v3.1.0: Self version
    if (infoVersion < 2)
        return;

    // Load data
    QString appVersion;

    stream >> appVersion;

    if (appVersion != APP_VERSION_STR)
        return; // app upgraded

    stream >> m_version >> m_releaseText >> m_downloadUrl >> m_downloadSize;

    emitAppVersionUpdated();
}

TaskUpdateChecker *TaskInfoUpdateChecker::updateChecker() const
{
    return static_cast<TaskUpdateChecker *>(taskWorker());
}

bool TaskInfoUpdateChecker::processResult(bool success)
{
    if (!success) {
        qCDebug(LC) << "Failed";
        return false;
    }

    const auto worker = updateChecker();

    if (m_version == worker->version()) {
        qCDebug(LC) << "Same version";
        return false;
    }

    m_version = worker->version();
    m_releaseText = worker->releaseText();
    m_downloadUrl = worker->downloadUrl();
    m_downloadSize = worker->downloadSize();

    emitAppVersionUpdated();

    if (isNewVersion()) {
        qCDebug(LC) << "New version found:" << m_version;

        emit taskManager()->appVersionDownloaded(m_version);
    } else {
        qCDebug(LC) << "Old version found:" << m_version;
    }

    return true;
}

void TaskInfoUpdateChecker::emitAppVersionUpdated()
{
    emit taskManager()->appVersionUpdated(m_version);
}
