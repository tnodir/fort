#ifndef TASKLISTMODELDATA_H
#define TASKLISTMODELDATA_H

#include <QModelIndex>
#include <QObject>

class TaskEditInfo;
class TaskInfo;

class TaskListModelData
{
public:
    explicit TaskListModelData(const TaskInfo *taskInfo, const TaskEditInfo &task,
            const QModelIndex &index = {}, int role = Qt::DisplayRole);

    int role() const { return m_role; }
    const QModelIndex &index() const { return m_index; }
    const TaskInfo *taskInfo() const { return m_taskInfo; }
    const TaskEditInfo &task() const { return m_task; }

    int column() const { return index().column(); }

    QVariant dataDecorationIcon() const;
    QVariant dataDisplayRow() const;

private:
    int m_role = Qt::DisplayRole;
    const QModelIndex &m_index;
    const TaskInfo *m_taskInfo = nullptr;
    const TaskEditInfo &m_task;
};

#endif // TASKLISTMODELDATA_H
