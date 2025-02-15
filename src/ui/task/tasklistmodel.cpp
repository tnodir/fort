#include "tasklistmodel.h"

#include <util/iconcache.h>

#include "taskinfo.h"
#include "tasklistmodeldata.h"
#include "tasklistmodelheaderdata.h"
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
        case Qt::ToolTipRole: {
            const TaskListModelHeaderData data(section, role);

            return data.headerDataDisplay();
        }

        // Icon
        case Qt::DecorationRole: {
            const TaskListModelHeaderData data(section, role);

            return data.headerDataDecoration();
        }
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
        return dataDecoration(index, role);

    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant TaskListModel::dataDisplay(const QModelIndex &index, int role) const
{
    const int row = index.row();

    const auto taskInfo = taskInfoAt(row);
    const auto &task = taskRowAt(row);

    const TaskListModelData data(taskInfo, task, index, role);

    return data.dataDisplayRow();
}

QVariant TaskListModel::dataDecoration(const QModelIndex &index, int role) const
{
    const int row = index.row();

    const auto &task = taskRowAt(row);

    const TaskListModelData data(/*taskInfo=*/nullptr, task, index, role);

    return data.dataDecorationIcon();
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

void TaskListModel::setTaskRowEdited(int row)
{
    const auto index = this->index(row, 0);
    const auto endIndex = this->index(row, columnCount() - 1);

    emitDataEdited(index, endIndex);
}

void TaskListModel::switchTaskEnabled(const QModelIndex &index)
{
    TaskEditInfo &taskRow = taskRowAt(index.row());

    taskRow.setEnabled(!taskRow.enabled());

    emitDataEdited(index, index, { Qt::CheckStateRole });
}

void TaskListModel::emitDataEdited(
        const QModelIndex &index, const QModelIndex &endIndex, const QList<int> &roles)
{
    emit dataChanged(index, endIndex, roles);

    emit dataEdited();
}
