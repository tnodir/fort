#include "taskmanager.h"

#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../util/dateutil.h"
#include "taskinfo.h"
#include "taskworker.h"

TaskManager::TaskManager(FortManager *fortManager,
                         QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
    setupTasks();

    m_timer.setSingleShot(true);

    connect(&m_timer, &QTimer::timeout,
            this, &TaskManager::runExpiredTasks);
}

QQmlListProperty<TaskInfo> TaskManager::taskInfos()
{
    return QQmlListProperty<TaskInfo>(this, m_taskInfos);
}

void TaskManager::setupTasks()
{
    appendTaskInfo(new TaskInfo(TaskInfo::UpdateChecker, this));
    appendTaskInfo(new TaskInfo(TaskInfo::Tasix, this));
}

void TaskManager::appendTaskInfo(TaskInfo *taskInfo)
{
    connect(taskInfo, &TaskInfo::workFinished,
            this, &TaskManager::handleTaskFinished);

    m_taskInfos.append(taskInfo);
}

void TaskManager::loadSettings(const FortSettings *fortSettings)
{
    const TasksMap tasksMap = fortSettings->tasks();

    foreach (TaskInfo *taskInfo, m_taskInfos) {
        const QString taskName = TaskInfo::typeToString(taskInfo->type());
        const QByteArray taskData = tasksMap.value(taskName);

        if (!taskData.isNull()) {
            taskInfo->setRawData(taskData);
        }
    }

    runExpiredTasks();
}

bool TaskManager::saveSettings(FortSettings *fortSettings)
{
    TasksMap tasksMap;
    QByteArray taskData;

    foreach (const TaskInfo *taskInfo, m_taskInfos) {
        const QString taskName = TaskInfo::typeToString(taskInfo->type());

        taskData.clear();
        taskInfo->rawData(taskData);

        tasksMap.insert(taskName, taskData);
    }

    runExpiredTasks();

    return fortSettings->setTasks(tasksMap);
}

void TaskManager::handleTaskFinished(bool success)
{
    if (success) {
        TaskInfo *taskInfo = qobject_cast<TaskInfo *>(sender());
        TaskWorker *taskWorker = taskInfo->taskWorker();

        taskWorker->processResult(m_fortManager);
    }

    saveSettings(m_fortManager->fortSettings());
}

void TaskManager::runExpiredTasks()
{
    const QDateTime now = DateUtil::now();
    bool enabledTaskExists = false;

    foreach (TaskInfo *taskInfo, m_taskInfos) {
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
