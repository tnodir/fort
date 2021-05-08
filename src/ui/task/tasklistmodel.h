#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include <QVector>

#include "../util/model/tableitemmodel.h"

class TaskInfo;
class TaskManager;

struct TaskRow
{
    explicit TaskRow() : edited(false), enabled(false), intervalHours(0) { }

    quint16 edited : 1;
    quint16 enabled : 1;
    quint16 intervalHours;
};

class TaskListModel : public TableItemModel
{
    Q_OBJECT

public:
    enum Roles { RoleEnabled = Qt::UserRole, RoleIntervalHours, RoleRunning };
    Q_ENUM(Roles)

    explicit TaskListModel(TaskManager *taskManager, QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool v);

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
    void saveChanges();

    void resetEdited();

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    void setupTaskRows();

    bool taskEnabled(int index) const;
    void setTaskEnabled(const QModelIndex &index, bool v);

    int taskIntervalHours(int index) const;
    void setTaskIntervalHours(const QModelIndex &index, int v);

    TaskRow &taskRowAt(int row) { return m_taskRows[row]; }
    const TaskRow &taskRowAt(int row) const { return m_taskRows[row]; }

    static QString formatDateTime(const QDateTime &dateTime);

private:
    bool m_edited = false;

    TaskManager *m_taskManager = nullptr;

    QVector<TaskRow> m_taskRows;
};

#endif // TASKLISTMODEL_H
