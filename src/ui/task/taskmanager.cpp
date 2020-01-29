#include "taskmanager.h"

#include "../conf/confmanager.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../util/dateutil.h"
#include "taskinfoupdatechecker.h"
#include "taskinfozonedownloader.h"

TaskManager::TaskManager(FortManager *fortManager,
                         QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
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

void TaskManager::setupTasks()
{
    m_taskInfoUpdateChecker = new TaskInfoUpdateChecker(this);

    appendTaskInfo(m_taskInfoUpdateChecker);
    appendTaskInfo(new TaskInfoZoneDownloader(this));
}

void TaskManager::appendTaskInfo(TaskInfo *taskInfo)
{
    connect(taskInfo, &TaskInfo::workStarted, this, &TaskManager::handleTaskStarted);
    connect(taskInfo, &TaskInfo::workFinished, this, &TaskManager::handleTaskFinished);

    m_taskInfos.append(taskInfo);
}

void TaskManager::loadSettings()
{
    if (!confManager()->loadTasks(m_taskInfos)) {
        const TasksMap tasksMap = settings()->tasks();
        if (!tasksMap.isEmpty()) {
            for (TaskInfo *taskInfo : m_taskInfos) {
                const QByteArray taskData = tasksMap.value(taskInfo->name());
                if (!taskData.isNull()) {
                    taskInfo->setRawData(taskData);
                }
            }
        }
    }

    runExpiredTasks();
}

bool TaskManager::saveSettings()
{
    runExpiredTasks();

    if (!confManager()->saveTasks(m_taskInfos))
        return false;

    settings()->removeTasks();
    return true;
}

void TaskManager::handleTaskStarted()
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    emit taskStarted(taskInfo);
}

void TaskManager::handleTaskFinished(bool success)
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    taskInfo->processResult(m_fortManager, success);

    saveSettings();

    emit taskFinished(taskInfo);
}

void TaskManager::runExpiredTasks()
{
    const QDateTime now = DateUtil::now();
    bool enabledTaskExists = false;

    for (TaskInfo *taskInfo : m_taskInfos) {
        if (!taskInfo->enabled())
            continue;

        enabledTaskExists = true;

        if (now >= taskInfo->plannedRun()) {
            taskInfo->run();
        }
    }

    if (enabledTaskExists) {
        m_timer.start(60 * 60 * 1000);  // 1 hour
    } else {
        m_timer.stop();
    }
}
