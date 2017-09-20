#include "taskmanager.h"

#include "../fortmanager.h"
#include "../fortsettings.h"
#include "taskinfo.h"

TaskManager::TaskManager(FortManager *fortManager,
                         QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
}

QQmlListProperty<TaskInfo> TaskManager::taskInfos()
{
    return QQmlListProperty<TaskInfo>(this, m_taskInfos);
}

void TaskManager::loadSettings(const FortSettings *fortSettings)
{
    m_taskInfos.append(new TaskInfo(TaskInfo::Tasix, this));

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
