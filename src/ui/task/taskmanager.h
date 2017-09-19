#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QQmlListProperty>

class FortManager;
class Task;

class TaskManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Task> tasks READ tasks NOTIFY tasksChanged)
    Q_CLASSINFO("DefaultProperty", "tasks")

public:
    explicit TaskManager(FortManager *fortManager,
                         QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }

    const QList<Task *> &tasksList() const { return m_tasks; }
    QQmlListProperty<Task> tasks();

signals:
    void tasksChanged();

public slots:

private:
    FortManager *m_fortManager;

    QList<Task *> m_tasks;
};

#endif // TASKMANAGER_H
