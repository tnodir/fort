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

int TaskListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
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
    case RoleEnabled:
        return dataCheckState(index);

    case RoleRunOnStartup:
        return taskRunOnStartup(index.row());

    case RoleIntervalHours:
        return taskIntervalHours(index.row());

    case RoleRunning:
        return taskRunning(index.row());
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
    case 1:
        return taskIntervalHours(row);
    case 2:
        return taskInfo->lastRun();
    case 3:
        return taskInfo->lastSuccess();
    }

    return QVariant();
}

QVariant TaskListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        return taskEnabled(index.row()) ? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool TaskListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        setTaskEnabled(index, !taskEnabled(index.row()));
        return true;

    case RoleEnabled:
        setTaskEnabled(index, value.toBool());
        return true;

    case RoleRunOnStartup:
        setTaskRunOnStartup(index, value.toBool());
        return true;

    case RoleIntervalHours:
        setTaskIntervalHours(index, value.toInt());
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
        taskRow.setRunOnStartup(taskInfo->runOnStatup());
        taskRow.setIntervalHours(taskInfo->intervalHours());
    }
}

QVariant TaskListModel::toVariant() const
{
    QVariantList list;
    for (const TaskEditInfo &info : std::as_const(m_taskRows)) {
        list.append(info.value());
    }
    return list;
}

bool TaskListModel::taskEnabled(int row) const
{
    const TaskEditInfo &taskRow = taskRowAt(row);
    return taskRow.enabled();
}

void TaskListModel::setTaskEnabled(const QModelIndex &index, bool v)
{
    TaskEditInfo &taskRow = taskRowAt(index);
    if (taskRow.enabled() == v)
        return;

    taskRow.setEnabled(v);

    emitDataEdited(index, Qt::CheckStateRole);
}

bool TaskListModel::taskRunOnStartup(int row) const
{
    const TaskEditInfo &taskRow = taskRowAt(row);
    return taskRow.runOnStartup();
}

void TaskListModel::setTaskRunOnStartup(const QModelIndex &index, bool v)
{
    TaskEditInfo &taskRow = taskRowAt(index);
    if (taskRow.runOnStartup() == v)
        return;

    taskRow.setRunOnStartup(v);

    emitDataEdited(index, Qt::DisplayRole);
}

int TaskListModel::taskIntervalHours(int row) const
{
    const TaskEditInfo &taskRow = taskRowAt(row);
    return taskRow.intervalHours();
}

void TaskListModel::setTaskIntervalHours(const QModelIndex &index, int v)
{
    TaskEditInfo &taskRow = taskRowAt(index);
    if (taskRow.intervalHours() == v)
        return;

    taskRow.setIntervalHours(v);

    emitDataEdited(index, Qt::DisplayRole);
}

bool TaskListModel::taskRunning(int row) const
{
    const auto taskInfo = taskInfoAt(row);
    return taskInfo->running();
}

void TaskListModel::emitDataEdited(const QModelIndex &index, int role)
{
    emit dataChanged(index, index, { role });
    emit dataEdited();
}
