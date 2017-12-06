#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QQmlListProperty>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TaskInfo)

class TaskManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<TaskInfo> taskInfos READ taskInfos NOTIFY taskInfosChanged)
    Q_CLASSINFO("DefaultProperty", "taskInfos")

public:
    explicit TaskManager(FortManager *fortManager,
                         QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }

    const QList<TaskInfo *> &taskInfosList() const { return m_taskInfos; }
    QQmlListProperty<TaskInfo> taskInfos();

signals:
    void taskInfosChanged();

public slots:
    void loadSettings(const FortSettings *fortSettings);
    bool saveSettings(FortSettings *fortSettings);

private slots:
    void handleTaskFinished(bool success);
    void runExpiredTasks();

private:
    void setupTasks();

    void appendTaskInfo(TaskInfo *taskInfo);

private:
    FortManager *m_fortManager;

    QList<TaskInfo *> m_taskInfos;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
