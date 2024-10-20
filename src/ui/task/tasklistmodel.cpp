#include "tasklistmodel.h"

#include <util/iconcache.h>

#include "taskinfo.h"
#include "taskmanager.h"

namespace {

inline QVariant headerDataDisplay(int column, int role)
{
    switch (column) {
    case 0:
        return TaskListModel::tr("Name");
    case 1:
        return TaskListModel::tr("Interval, hours");
    case 2:
        return (role == Qt::ToolTipRole) ? TaskListModel::tr("Run On Startup") : QString();
    case 3:
        return (role == Qt::ToolTipRole) ? TaskListModel::tr("Maximum retries count") : QString();
    case 4:
        return TaskListModel::tr("Last Run");
    case 5:
        return TaskListModel::tr("Last Success");
    }

    return {};
}

inline QVariant headerDataDecoration(int column)
{
    switch (column) {
    case 2:
        return IconCache::icon(":/icons/play.png");
    case 3:
        return IconCache::icon(":/icons/arrow_rotate_clockwise.png");
    }

    return {};
}

}

TaskListModel::TaskListModel(TaskManager *taskManager, QObject *parent) :
    TableItemModel(parent), m_taskManager(taskManager)
{
    setupTaskRows();

    connect(m_taskManager, &TaskManager::taskStarted, this, &TaskListModel::refresh);
    connect(m_taskManager, &TaskManager::taskFinished, this, &TaskListModel::refresh);
}

const QList<TaskInfo *> &TaskListModel::taskInfoList() const
{
    return taskManager()->taskInfoList();
}

TaskInfo *TaskListModel::taskInfoAt(int row) const
{
    return taskInfoList().at(row);
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return taskInfoList().size();
}

int TaskListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 6;
}

QVariant TaskListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        // Label
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return headerDataDisplay(section, role);

        // Icon
        case Qt::DecorationRole:
            return headerDataDecoration(section);
        }
    }
    return {};
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index, role);

        // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);

    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant TaskListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int column = index.column();

    const auto taskInfo = taskInfoAt(row);
    const auto &task = taskRowAt(row);

    switch (column) {
    case 0:
        return taskInfo->title();
    case 1:
        return task.intervalHours();
    case 2:
        return (role == Qt::ToolTipRole) ? taskStartupText(task) : QString();
    case 3:
        return (role == Qt::ToolTipRole) ? taskRetryText(task) : QString();
    case 4:
        return taskInfo->lastRun();
    case 5:
        return taskInfo->lastSuccess();
    }

    return {};
}

QVariant TaskListModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &task = taskRowAt(row);

    switch (column) {
    case 2:
        return task.runOnStartup() ? IconCache::icon(":/icons/play.png") : QIcon();
    case 3:
        return task.maxRetries() > 0 ? IconCache::icon(":/icons/arrow_rotate_clockwise.png")
                                     : QIcon();
    }

    return {};
}

QVariant TaskListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &task = taskRowAt(index.row());
        return task.enabled() ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

QString TaskListModel::taskStartupText(const TaskEditInfo &task) const
{
    if (!task.runOnStartup())
        return {};

    return QString("(%1)").arg(task.delayStartup());
}

QString TaskListModel::taskRetryText(const TaskEditInfo &task) const
{
    if (task.maxRetries() == 0)
        return {};

    QString text = QString::number(task.maxRetries());

    if (task.retrySeconds() > 0) {
        text += QString(" (%1)").arg(task.retrySeconds());
    }

    return text;
}

bool TaskListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        switchTaskEnabled(index);
        return true;
    }

    return false;
}

Qt::ItemFlags TaskListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

void TaskListModel::setupTaskRows()
{
    const int taskCount = taskInfoList().size();
    m_taskRows.resize(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        const TaskInfo *taskInfo = taskInfoAt(i);

        TaskEditInfo &taskRow = taskRowAt(i);
        taskRow.setEnabled(taskInfo->enabled());
        taskRow.setRunOnStartup(taskInfo->runOnStartup());
        taskRow.setDelayStartup(taskInfo->delayStartup());
        taskRow.setMaxRetries(taskInfo->maxRetries());
        taskRow.setRetrySeconds(taskInfo->retrySeconds());
        taskRow.setIntervalHours(taskInfo->intervalHours());
    }
}

QVariant TaskListModel::toVariant() const
{
    QVariantList list;
    for (const TaskEditInfo &task : std::as_const(m_taskRows)) {
        list.append(task.value());
    }
    return list;
}

void TaskListModel::setTaskRowEdited(int row, int role)
{
    const auto index = this->index(row, 0);
    const auto endIndex = this->index(row, columnCount() - 1);

    emitDataEdited(index, endIndex, role);
}

void TaskListModel::switchTaskEnabled(const QModelIndex &index)
{
    TaskEditInfo &taskRow = taskRowAt(index.row());

    taskRow.setEnabled(!taskRow.enabled());

    emitDataEdited(index, index, Qt::CheckStateRole);
}

void TaskListModel::emitDataEdited(const QModelIndex &index, const QModelIndex &endIndex, int role)
{
    emit dataChanged(index, endIndex, { role });

    emit dataEdited();
}
