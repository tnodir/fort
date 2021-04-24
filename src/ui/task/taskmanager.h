#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QTimer>

class ConfManager;
class FortManager;
class FortSettings;
class TaskInfo;
class TaskInfoUpdateChecker;
class TaskInfoZoneDownloader;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;

    TaskInfoUpdateChecker *taskInfoUpdateChecker() const;
    TaskInfoZoneDownloader *taskInfoZoneDownloader() const;

    const QList<TaskInfo *> &taskInfosList() const { return m_taskInfos; }

    virtual void initialize();

signals:
    void taskInfosChanged();

    void taskStarted(TaskInfo *taskInfo);
    void taskFinished(TaskInfo *taskInfo);

public slots:
    void loadSettings();
    bool saveSettings();

private slots:
    void handleTaskStarted();
    void handleTaskFinished(bool success);

    void runExpiredTasks();

private:
    void setupTasks();

    void appendTaskInfo(TaskInfo *taskInfo);

private:
    FortManager *m_fortManager = nullptr;

    QList<TaskInfo *> m_taskInfos;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
