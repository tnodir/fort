#include "taskinfoupdatechecker.h"

#include <QDataStream>

#include <fort_version.h>

#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

#include "taskupdatechecker.h"

#define TASK_INFO_VERSION 2

TaskInfoUpdateChecker::TaskInfoUpdateChecker(TaskManager &taskManager) :
    TaskInfo(UpdateChecker, taskManager)
{
}

bool TaskInfoUpdateChecker::isNewVersion() const
{
    return !version().isEmpty() && version() != APP_VERSION_STR;
}

QByteArray TaskInfoUpdateChecker::data() const
{
    QByteArray data;
    QDataStream stream(&data, QDataStream::WriteOnly);

    // Store data
    const quint16 infoVersion = TASK_INFO_VERSION;

    stream << infoVersion << QString::fromLatin1(APP_VERSION_STR) << m_version << m_downloadUrl
           << m_releaseText;

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

    stream >> m_version >> m_downloadUrl >> m_releaseText;

    emit versionChanged();
}

TaskUpdateChecker *TaskInfoUpdateChecker::updateChecker() const
{
    return static_cast<TaskUpdateChecker *>(taskWorker());
}

bool TaskInfoUpdateChecker::processResult(bool success)
{
    if (!success)
        return false;

    const auto worker = updateChecker();

    if (m_version == worker->version())
        return false;

    m_version = worker->version();
    m_downloadUrl = worker->downloadUrl();
    m_releaseText = worker->releaseText();

    emit versionChanged();

    if (isNewVersion()) {
        IoC<WindowManager>()->showTrayMessage(tr("New version v%1 available!").arg(m_version));
    }

    return true;
}
