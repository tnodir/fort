#include "tasklistmodeldata.h"

#include <QFont>

#include <util/iconcache.h>

#include "taskinfo.h"
#include "tasklistmodel.h"

namespace {

QVariant dataDisplayName(const TaskInfo *taskInfo, const TaskEditInfo & /*task*/, int /*role*/)
{
    return taskInfo->title();
}

QVariant dataDisplayInterval(const TaskInfo *taskInfo, const TaskEditInfo & /*task*/, int /*role*/)
{
    return taskInfo->intervalHours();
}

QVariant dataDisplayStartup(const TaskInfo * /*taskInfo*/, const TaskEditInfo &task, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    if (!task.runOnStartup())
        return {};

    return QString("(%1)").arg(task.delayStartup());
}

QVariant dataDisplayRetry(const TaskInfo * /*taskInfo*/, const TaskEditInfo &task, int role)
{
    if (role != Qt::ToolTipRole)
        return {};

    if (task.maxRetries() == 0)
        return {};

    QString text = QString::number(task.maxRetries());

    if (task.retrySeconds() > 0) {
        text += QString(" (%1)").arg(task.retrySeconds());
    }

    return text;
}

QVariant dataDisplayLastRun(const TaskInfo *taskInfo, const TaskEditInfo & /*task*/, int /*role*/)
{
    return taskInfo->lastRun();
}

QVariant dataDisplayLastSuccess(
        const TaskInfo *taskInfo, const TaskEditInfo & /*task*/, int /*role*/)
{
    return taskInfo->lastSuccess();
}

using dataDisplay_func = QVariant (*)(const TaskInfo *taskInfo, const TaskEditInfo &task, int role);

static const dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayName,
    &dataDisplayInterval,
    &dataDisplayStartup,
    &dataDisplayRetry,
    &dataDisplayLastRun,
    &dataDisplayLastSuccess,
};

}

TaskListModelData::TaskListModelData(
        const TaskInfo *taskInfo, const TaskEditInfo &task, const QModelIndex &index, int role) :
    m_role(role), m_index(index), m_taskInfo(taskInfo), m_task(task)
{
}

QVariant TaskListModelData::dataDecorationIcon() const
{

    switch (column()) {
    case 2:
        return task().runOnStartup() ? IconCache::icon(":/icons/play.png") : QIcon();
    case 3:
        return task().maxRetries() > 0 ? IconCache::icon(":/icons/arrow_rotate_clockwise.png")
                                       : QIcon();
    }

    return {};
}

QVariant TaskListModelData::dataDisplayRow() const
{
    const dataDisplay_func func = dataDisplay_funcList[column()];

    return func(taskInfo(), task(), role());
}
