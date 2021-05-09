#include "tasklistmodel.h"

#include "taskinfo.h"
#include "taskmanager.h"

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
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        switch (section) {
        case 0:
            return tr("Name");
        case 1:
            return tr("Interval, hours");
        case 2:
            return tr("Last Run");
        case 3:
            return tr("Last Success");
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

    case RoleIntervalHours:
        return taskIntervalHours(index.row());

    case RoleRunning: {
        const auto taskInfo = taskInfoAt(index.row());
        return taskInfo->running();
    }
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
        return formatDateTime(taskInfo->lastRun());
    case 3:
        return formatDateTime(taskInfo->lastSuccess());
    }

    return QVariant();
}

QVariant TaskListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        return taskEnabled(index.row());
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

    case RoleIntervalHours:
        setTaskIntervalHours(index, value.toInt());
        return true;
    }

    return false;
}

Qt::ItemFlags TaskListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int column = index.column();

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren
            | (column == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags);
}

void TaskListModel::setupTaskRows()
{
    const int taskCount = taskInfoList().size();
    m_taskRows.resize(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        const TaskInfo *taskInfo = taskInfoAt(i);

        TaskEditInfo &taskRow = taskRowAt(i);
        taskRow.setEnabled(taskInfo->enabled());
        taskRow.setIntervalHours(taskInfo->intervalHours());
    }
}

QVariant TaskListModel::toVariant() const
{
    QVariantList list;
    for (const TaskInfo *task : taskInfoList()) {
        list.append(task->editToVariant());
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
    const int row = index.row();

    TaskEditInfo &taskRow = taskRowAt(row);
    if (taskRow.enabled() == v)
        return;

    taskRow.setEnabled(v);

    emit dataChanged(index, index, { Qt::CheckStateRole });
    emit dataEdited();
}

int TaskListModel::taskIntervalHours(int row) const
{
    const TaskEditInfo &taskRow = taskRowAt(row);
    return taskRow.intervalHours();
}

void TaskListModel::setTaskIntervalHours(const QModelIndex &index, int v)
{
    const int row = index.row();

    TaskEditInfo &taskRow = taskRowAt(row);
    if (taskRow.intervalHours() == v)
        return;

    taskRow.setIntervalHours(v);

    emit dataChanged(index, index, { Qt::DisplayRole });
    emit dataEdited();
}

QString TaskListModel::formatDateTime(const QDateTime &dateTime)
{
    return dateTime.toString("yyyy-MM-dd HH:mm:ss");
}
