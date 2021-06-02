#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QVariant>

#include "../util/ioc/iocservice.h"

class TaskInfo;
class TaskInfoUpdateChecker;
class TaskInfoZoneDownloader;

class TaskManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = nullptr);

    TaskInfoUpdateChecker *taskInfoUpdateChecker() const;
    TaskInfoZoneDownloader *taskInfoZoneDownloader() const;

    const QList<TaskInfo *> &taskInfoList() const { return m_taskInfoList; }
    TaskInfo *taskInfoAt(int row) const;

    void setUp() override;

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
    QList<TaskInfo *> m_taskInfoList;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
