#include "taskmanager.h"

#include "../conf/confmanager.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../util/dateutil.h"
#include "taskinfoupdatechecker.h"
#include "taskinfozonedownloader.h"

TaskManager::TaskManager(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    setupTasks();

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &TaskManager::runExpiredTasks);
}

FortSettings *TaskManager::settings() const
{
    return fortManager()->settings();
}

ConfManager *TaskManager::confManager() const
{
    return fortManager()->confManager();
}

TaskInfoUpdateChecker *TaskManager::taskInfoUpdateChecker() const
{
    return static_cast<TaskInfoUpdateChecker *>(m_taskInfos.at(0));
}

TaskInfoZoneDownloader *TaskManager::taskInfoZoneDownloader() const
{
    return static_cast<TaskInfoZoneDownloader *>(m_taskInfos.at(1));
}

void TaskManager::initialize()
{
    loadSettings();

    taskInfoZoneDownloader()->loadZones();

    m_timer.start(5 * 1000); // 5 seconds
}

void TaskManager::setupTasks()
{
    appendTaskInfo(new TaskInfoUpdateChecker(*this));
    appendTaskInfo(new TaskInfoZoneDownloader(*this));
}

void TaskManager::appendTaskInfo(TaskInfo *taskInfo)
{
    connect(taskInfo, &TaskInfo::workStarted, this, &TaskManager::handleTaskStarted);
    connect(taskInfo, &TaskInfo::workFinished, this, &TaskManager::handleTaskFinished);

    m_taskInfos.append(taskInfo);
}

void TaskManager::loadSettings()
{
    confManager()->loadTasks(m_taskInfos);
}

bool TaskManager::saveSettings()
{
    runExpiredTasks();

    return confManager()->saveTasks(m_taskInfos);
}

void TaskManager::handleTaskStarted()
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    emit taskStarted(taskInfo);
}

void TaskManager::handleTaskFinished(bool success)
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    taskInfo->processResult(success);

    saveSettings();

    emit taskFinished(taskInfo);
}

void TaskManager::runExpiredTasks()
{
    const QDateTime now = DateUtil::now();
    bool enabledTaskExists = false;

    for (TaskInfo *taskInfo : qAsConst(m_taskInfos)) {
        if (!taskInfo->enabled())
            continue;

        enabledTaskExists = true;

        if (now >= taskInfo->plannedRun()) {
            taskInfo->run();
        }
    }

    if (enabledTaskExists) {
        m_timer.start(60 * 60 * 1000); // 1 hour
    } else {
        m_timer.stop();
    }
}
