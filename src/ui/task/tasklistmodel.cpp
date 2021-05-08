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

void TaskListModel::setEdited(bool v)
{
    if (m_edited != v) {
        m_edited = v;

        if (edited()) {
            emit dataEdited();
        }
    }
}

const QList<TaskInfo *> &TaskListModel::taskInfosList() const
{
    return taskManager()->taskInfosList();
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return taskInfosList().size();
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

void TaskListModel::saveChanges()
{
    const int taskCount = taskInfosList().size();
    for (int i = 0; i < taskCount; ++i) {
        const TaskRow &taskRow = taskRowAt(i);
        if (!taskRow.edited)
            continue;

        TaskInfo *taskInfo = taskInfoAt(i);
        taskInfo->setEnabled(taskRow.enabled);
        taskInfo->setIntervalHours(taskRow.intervalHours);
    }

    taskManager()->saveSettings();
}

void TaskListModel::resetEdited()
{
    beginResetModel();
    setupTaskRows();
    endResetModel();
}

void TaskListModel::setupTaskRows()
{
    const int taskCount = taskInfosList().size();
    m_taskRows.resize(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        const TaskInfo *taskInfo = taskInfoAt(i);

        TaskRow &taskRow = taskRowAt(i);
        taskRow.edited = false;
        taskRow.enabled = taskInfo->enabled();
        taskRow.intervalHours = taskInfo->intervalHours();
    }

    setEdited(false);
}

bool TaskListModel::taskEnabled(int index) const
{
    const auto taskRow = taskRowAt(index);
    return taskRow.enabled;
}

void TaskListModel::setTaskEnabled(const QModelIndex &index, bool v)
{
    const int row = index.row();

    auto taskRow = taskRowAt(row);
    if (taskRow.enabled == v)
        return;

    taskRow.edited = true;
    taskRow.enabled = v;

    emit dataChanged(index, index, { Qt::CheckStateRole });
    setEdited(true);
}

int TaskListModel::taskIntervalHours(int index) const
{
    const auto taskRow = taskRowAt(index);
    return taskRow.intervalHours;
}

void TaskListModel::setTaskIntervalHours(const QModelIndex &index, int v)
{
    const int row = index.row();

    auto taskRow = taskRowAt(row);
    if (taskRow.intervalHours == v)
        return;

    taskRow.edited = true;
    taskRow.intervalHours = v;

    emit dataChanged(index, index, { Qt::DisplayRole });
    setEdited(true);
}

TaskInfo *TaskListModel::taskInfoAt(int row) const
{
    return taskInfosList().at(row);
}

QString TaskListModel::formatDateTime(const QDateTime &dateTime)
{
    return dateTime.toString("yyyy-MM-dd HH:mm:ss");
}
