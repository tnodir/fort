#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include <QList>

#include "../util/model/tableitemmodel.h"

QT_FORWARD_DECLARE_CLASS(TaskInfo)
QT_FORWARD_DECLARE_CLASS(TaskManager)

struct TaskRow
{
    bool enabled = false;
    int intervalHours = 0;
};

class TaskListModel : public TableItemModel
{
    Q_OBJECT

public:
    enum Roles { RoleEnabled = Qt::UserRole, RoleIntervalHours, RoleRunning };
    Q_ENUM(Roles)

    explicit TaskListModel(TaskManager *taskManager, QObject *parent = nullptr);
    ~TaskListModel() override;

    TaskManager *taskManager() const { return m_taskManager; }

    const QList<TaskInfo *> &taskInfosList() const;
    TaskInfo *taskInfoAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
    void dataEdited();

public slots:
    void resetEdited();

    void saveChanges();

private:
    void setupTaskRows();
    void clearTaskRows();

    bool taskEnabled(int index) const;
    void setTaskEnabled(const QModelIndex &index, bool v);

    int taskIntervalHours(int index) const;
    void setTaskIntervalHours(const QModelIndex &index, int v);

    TaskRow *addTaskRow(int row);

    TaskRow *taskRowAt(int row) const;

    static QString formatDateTime(const QDateTime &dateTime);

private:
    TaskManager *m_taskManager = nullptr;

    mutable QList<TaskRow *> m_taskRows;
};

#endif // TASKLISTMODEL_H
