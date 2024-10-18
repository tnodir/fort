#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include <QVector>

#include <util/model/tableitemmodel.h>

#include "taskeditinfo.h"

class TaskInfo;
class TaskManager;

class TaskListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit TaskListModel(TaskManager *taskManager, QObject *parent = nullptr);

    TaskManager *taskManager() const { return m_taskManager; }

    const QList<TaskInfo *> &taskInfoList() const;
    TaskInfo *taskInfoAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void setupTaskRows();

    QVariant toVariant() const;

    void setTaskRowEdited(int row, int role = Qt::DisplayRole);

    TaskEditInfo &taskRowAt(int row) { return m_taskRows[row]; }
    const TaskEditInfo &taskRowAt(int row) const { return m_taskRows[row]; }

signals:
    void dataEdited();

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash & /*vars*/, int /*row*/) const override { return true; }
    TableRow &tableRow() const override { return m_taskRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    void switchTaskEnabled(const QModelIndex &index);

    bool taskRunning(int row) const;

    void emitDataEdited(const QModelIndex &index, const QModelIndex &endIndex, int role);

private:
    TaskManager *m_taskManager = nullptr;

    QVector<TaskEditInfo> m_taskRows;

    mutable TableRow m_taskRow;
};

#endif // TASKLISTMODEL_H
