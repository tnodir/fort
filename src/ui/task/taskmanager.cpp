#include "taskmanager.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>
#include <util/dateutil.h>

#include "taskinfoapppurger.h"
#include "taskinfoupdatechecker.h"
#include "taskinfozonedownloader.h"

using namespace Fort;

namespace {

constexpr int TIMER_STARTUP_SECONDS = 1;
constexpr int TIMER_DEFAULT_SECONDS = 5;
constexpr int TIMER_MAX_SECONDS = 24 * 60 * 60; // 1 day

}

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
    auto confManager = Fort::dependency<ConfManager>();

    loadSettings();
    initializeTasks();

    setupTimer(TIMER_STARTUP_SECONDS);

    connect(confManager, &ConfManager::confChanged, this, [&](bool onlyFlags, uint editedFlags) {
        if (onlyFlags && (editedFlags & FirewallConf::TaskEdited) == 0)
            return;

        loadSettings();

        runExpiredTasks();
    });
}

void TaskManager::initializeTasks()
{
    for (TaskInfo *taskInfo : taskInfoList()) {
        taskInfo->initialize();
    }
}

void TaskManager::setupTimer(int secs)
{
    m_timer.stop();

    if (secs >= 0) {
        m_timer.start(secs * 1000);
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
    confManager()->loadTasks(taskInfoList());
}

bool TaskManager::saveSettings()
{
    return confManager()->saveTasks(taskInfoList());
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
    qint64 sleepSecs = -1;

    for (TaskInfo *taskInfo : taskInfoList()) {
        qint64 secsToRun;
        if (runExpiredTask(taskInfo, now, secsToRun)) {
            sleepSecs = (sleepSecs < 0) ? secsToRun : qMin(sleepSecs, secsToRun);
        }
    }

    m_isFirstRun = false;

    const int secs = qMin(sleepSecs, TIMER_MAX_SECONDS);

    setupTimer(secs);
}

bool TaskManager::runExpiredTask(TaskInfo *taskInfo, const QDateTime &now, qint64 &secsToRun)
{
    if (!taskInfo->enabled())
        return false;

    if (taskInfo->running()) {
        secsToRun = TIMER_DEFAULT_SECONDS;
        return true;
    }

    secsToRun = taskInfo->secondsToRun(now, m_isFirstRun);

    if (secsToRun <= 1) {
        secsToRun = TIMER_DEFAULT_SECONDS;

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
