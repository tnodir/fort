#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QQmlListProperty>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TaskInfo)
QT_FORWARD_DECLARE_CLASS(TaskInfoUpdateChecker)

class TaskManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<TaskInfo> taskInfos READ taskInfos NOTIFY taskInfosChanged)
    Q_CLASSINFO("DefaultProperty", "taskInfos")

public:
    explicit TaskManager(FortManager *fortManager,
                         QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }

    TaskInfoUpdateChecker *taskInfoUpdateChecker() const { return m_taskInfoUpdateChecker; }

    const QList<TaskInfo *> &taskInfosList() const { return m_taskInfos; }
    QQmlListProperty<TaskInfo> taskInfos();

signals:
    void taskInfosChanged();

public slots:
    void loadSettings(const FortSettings *fortSettings, ConfManager *confManager);
    bool saveSettings(FortSettings *fortSettings, ConfManager *confManager);

private slots:
    void handleTaskFinished(bool success);

    void runExpiredTasks();

private:
    void setupTasks();

    void appendTaskInfo(TaskInfo *taskInfo);

private:
    FortManager *m_fortManager;

    TaskInfoUpdateChecker *m_taskInfoUpdateChecker;
    QList<TaskInfo *> m_taskInfos;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
