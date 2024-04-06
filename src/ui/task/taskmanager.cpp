#include "taskmanager.h"

#include <conf/confmanager.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "taskinfoapppurger.h"
#include "taskinfoupdatechecker.h"
#include "taskinfozonedownloader.h"

TaskManager::TaskManager(QObject *parent) : QObject(parent)
{
    setupTasks();

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &TaskManager::runExpiredTasks);
}

TaskInfoUpdateChecker *TaskManager::taskInfoUpdateChecker() const
{
    return static_cast<TaskInfoUpdateChecker *>(taskInfoAt(TaskInfo::UpdateChecker));
}

TaskInfoZoneDownloader *TaskManager::taskInfoZoneDownloader() const
{
    return static_cast<TaskInfoZoneDownloader *>(taskInfoAt(TaskInfo::ZoneDownloader));
}

TaskInfoAppPurger *TaskManager::taskInfoAppPurger() const
{
    return static_cast<TaskInfoAppPurger *>(taskInfoAt(TaskInfo::AppPurger));
}

TaskInfo *TaskManager::taskInfoAt(int row) const
{
    return taskInfoList().at(row);
}

void TaskManager::setUp()
{
    loadSettings();

    setupTimer();
}

void TaskManager::setupTimer(bool enabled)
{
    if (enabled) {
        const int msecs = m_isFirstRun ? 1000 // 1 second
                                       : 5 * 60 * 1000; // 5 minutes
        m_timer.start(msecs);
    } else {
        m_timer.stop();
    }
}

void TaskManager::setupTasks()
{
    appendTaskInfo(new TaskInfoUpdateChecker(*this));
    appendTaskInfo(new TaskInfoZoneDownloader(*this));
    appendTaskInfo(new TaskInfoAppPurger(*this));
}

void TaskManager::appendTaskInfo(TaskInfo *taskInfo)
{
    connect(taskInfo, &TaskInfo::workStarted, this, &TaskManager::handleTaskStarted);
    connect(taskInfo, &TaskInfo::workFinished, this, &TaskManager::handleTaskFinished);

    m_taskInfoList.append(taskInfo);
}

void TaskManager::loadSettings()
{
    auto confManager = IoCDependency<ConfManager>();

    confManager->loadTasks(taskInfoList());
}

bool TaskManager::saveSettings()
{
    return IoC<ConfManager>()->saveTasks(taskInfoList());
}

bool TaskManager::saveVariant(const QVariant &v)
{
    const QVariantList list = v.toList();

    const int taskCount = taskInfoList().size();
    for (int i = 0; i < taskCount; ++i) {
        TaskInfo *task = taskInfoAt(i);
        task->editFromVariant(list.value(i));
    }

    return saveSettings();
}

void TaskManager::runTask(qint8 taskType)
{
    auto taskInfo = taskInfoByType(taskType);
    if (Q_LIKELY(taskInfo)) {
        taskInfo->run();
    }
}

void TaskManager::abortTask(qint8 taskType)
{
    auto taskInfo = taskInfoByType(taskType);
    if (Q_LIKELY(taskInfo)) {
        taskInfo->abortTask();
    }
}

void TaskManager::handleTaskStarted()
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    emit taskStarted(taskInfo->type());
}

void TaskManager::handleTaskFinished(bool success)
{
    auto taskInfo = qobject_cast<TaskInfo *>(sender());

    taskInfo->processResult(success);

    saveSettings();

    emit taskFinished(taskInfo->type());
}

void TaskManager::runExpiredTasks()
{
    const QDateTime now = DateUtil::now();
    bool enabledTaskExists = false;

    for (TaskInfo *taskInfo : taskInfoList()) {
        if (runExpiredTask(taskInfo, now)) {
            enabledTaskExists = true;
        }
    }

    m_isFirstRun = false;

    setupTimer(enabledTaskExists);
}

bool TaskManager::runExpiredTask(TaskInfo *taskInfo, const QDateTime &now)
{
    if (!taskInfo->enabled())
        return false;

    if ((m_isFirstRun && taskInfo->runOnStatup()) || now >= taskInfo->plannedRun()) {
        taskInfo->run();
    }

    return true;
}

TaskInfo *TaskManager::taskInfoByType(qint8 taskType) const
{
    switch (taskType) {
    case TaskInfo::UpdateChecker:
        return taskInfoUpdateChecker();
    case TaskInfo::ZoneDownloader:
        return taskInfoZoneDownloader();
    case TaskInfo::AppPurger:
        return taskInfoAppPurger();
    default:
        Q_UNREACHABLE();
        return nullptr;
    }
}
