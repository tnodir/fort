#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QVariant>

class ConfManager;
class FortManager;
class TaskInfo;
class TaskInfoUpdateChecker;
class TaskInfoZoneDownloader;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    ConfManager *confManager() const;

    TaskInfoUpdateChecker *taskInfoUpdateChecker() const;
    TaskInfoZoneDownloader *taskInfoZoneDownloader() const;

    const QList<TaskInfo *> &taskInfoList() const { return m_taskInfoList; }
    TaskInfo *taskInfoAt(int row) const;

    void initialize();

signals:
    void taskStarted(qint8 taskType);
    void taskFinished(qint8 taskType);

    void taskDoubleClicked(qint8 taskType);

public slots:
    void loadSettings();
    bool saveSettings();

    bool saveVariant(const QVariant &v);

    virtual void runTask(qint8 taskType);
    virtual void abortTask(qint8 taskType);

private slots:
    void handleTaskStarted();
    void handleTaskFinished(bool success);

    void runExpiredTasks();

protected:
    virtual void setupScheduler();

    TaskInfo *taskInfoByType(qint8 taskType) const;

private:
    void setupTasks();

    void appendTaskInfo(TaskInfo *taskInfo);

private:
    FortManager *m_fortManager = nullptr;

    QList<TaskInfo *> m_taskInfoList;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
