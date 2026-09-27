#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariant>

#include <util/ioc/iocservice.h>

class ConfManager;
class TaskInfo;
class TaskInfoAppPurger;
class TaskInfoUpdateChecker;
class TaskInfoZoneDownloader;

class TaskManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = nullptr);

    TaskInfoUpdateChecker *taskInfoUpdateChecker() const;
    TaskInfoZoneDownloader *taskInfoZoneDownloader() const;
    TaskInfoAppPurger *taskInfoAppPurger() const;

    const QList<TaskInfo *> &taskInfoList() const { return m_taskInfoList; }
    TaskInfo *taskInfoAt(int row) const;

    void setUp() override;

signals:
    void taskStarted(qint8 taskType);
    void taskFinished(qint8 taskType);

    void taskDoubleClicked(qint8 taskType);

    void appVersionUpdated();
    void appVersionDownloaded(const QString &version);

    void zonesUpdated(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);
    void zonesDownloaded(const QStringList &zoneNames);

public slots:
    void loadSettings();
    bool saveSettings();

    bool saveVariant(const QVariant &v);

    virtual void runTask(qint8 taskType);
    virtual void abortTask(qint8 taskType);

protected slots:
    void handleTaskStarted();
    void handleTaskFinished(bool success);

    virtual void runExpiredTasks();

protected:
    virtual void initializeTasks();

    virtual void setupTimer(int secs);

    virtual void setupConfImport(ConfManager *confManager);

    TaskInfo *taskInfoByType(qint8 taskType) const;

private:
    void setupTasks();

    void appendTaskInfo(TaskInfo *taskInfo);

    bool runExpiredTask(
            TaskInfo *taskInfo, qint64 &secsToRun, const QDateTime &now = {}, bool force = false);

private:
    bool m_isFirstRun = true;

    QList<TaskInfo *> m_taskInfoList;

    QTimer m_timer;
};

#endif // TASKMANAGER_H
