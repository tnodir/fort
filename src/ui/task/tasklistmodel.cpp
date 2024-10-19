#include "tasklistmodel.h"

#include "taskinfo.h"
#include "taskmanager.h"

namespace {

inline QVariant headerDataDisplay(int column)
{
    switch (column) {
    case 0:
        return TaskListModel::tr("Name");
    case 1:
        return TaskListModel::tr("Interval, hours");
    case 2:
        return TaskListModel::tr("Last Run");
    case 3:
        return TaskListModel::tr("Last Success");
    }
    return QVariant();
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
    return 4;
}

QVariant TaskListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        // Label
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return headerDataDisplay(section);
        }
    }
    return QVariant();
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return QVariant();
}

QVariant TaskListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto taskInfo = taskInfoAt(row);

    switch (column) {
    case 0:
        return taskInfo->title();
    case 1: {
        const auto &task = taskRowAt(row);
        return task.intervalHours();
    }
    case 2:
        return taskInfo->lastRun();
    case 3:
        return taskInfo->lastSuccess();
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
