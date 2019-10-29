#include "taskmanager.h"

#include "../conf/confmanager.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../util/dateutil.h"
#include "taskinfotasix.h"
#include "taskinfoupdatechecker.h"
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
    return {this, m_taskInfos};
}

void TaskManager::setupTasks()
{
    appendTaskInfo(new TaskInfoUpdateChecker(this));
    appendTaskInfo(new TaskInfoTasix(this));
}

void TaskManager::appendTaskInfo(TaskInfo *taskInfo)
{
    connect(taskInfo, &TaskInfo::workFinished,
            this, &TaskManager::handleTaskFinished);

    m_taskInfos.append(taskInfo);
}

void TaskManager::loadSettings(const FortSettings *fortSettings,
                               ConfManager *confManager)
{
    if (!confManager->loadTasks(m_taskInfos)) {
        const TasksMap tasksMap = fortSettings->tasks();
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

bool TaskManager::saveSettings(FortSettings *fortSettings,
                               ConfManager *confManager)
{
    runExpiredTasks();

    if (!confManager->saveTasks(m_taskInfos))
        return false;

    fortSettings->removeTasks();
    return true;
}

void TaskManager::handleTaskFinished(bool success)
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    taskInfo->processResult(m_fortManager, success);

    saveSettings(m_fortManager->fortSettings(),
                 m_fortManager->confManager());
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
