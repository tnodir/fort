#include "taskmanager.h"

#include "../fortmanager.h"
#include "../fortsettings.h"
#include "taskinfo.h"
#include "taskworker.h"

TaskManager::TaskManager(FortManager *fortManager,
                         QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
    setupTasks();
}

QQmlListProperty<TaskInfo> TaskManager::taskInfos()
{
    return QQmlListProperty<TaskInfo>(this, m_taskInfos);
}

void TaskManager::setupTasks()
{
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
}

bool TaskManager::saveSettings(FortSettings *fortSettings) const
{
    TasksMap tasksMap;
    QByteArray taskData;

    foreach (const TaskInfo *taskInfo, m_taskInfos) {
        const QString taskName = TaskInfo::typeToString(taskInfo->type());

        taskData.clear();
        taskInfo->rawData(taskData);

        tasksMap.insert(taskName, taskData);
    }

    return fortSettings->setTasks(tasksMap);
}

void TaskManager::handleTaskFinished(bool success)
{
    TaskInfo *taskInfo = qobject_cast<TaskInfo *>(sender());

    if (success) {
        TaskWorker *taskWorker = taskInfo->taskWorker();
        taskWorker->processResult(m_fortManager);
    }

    saveSettings(m_fortManager->fortSettings());
}
