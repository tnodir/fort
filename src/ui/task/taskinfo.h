#ifndef TASKINFO_H
#define TASKINFO_H

#include <QDateTime>
#include <QObject>

#include <util/classhelpers.h>

class TaskManager;
class TaskWorker;

class TaskInfo : public QObject
{
    Q_OBJECT

public:
    enum TaskType : qint8 {
        TypeNone = -1,
        UpdateChecker = 0,
        ZoneDownloader,
        AppPurger,
    };
    Q_ENUM(TaskType)

    explicit TaskInfo(TaskInfo::TaskType type, TaskManager &taskManager);
    ~TaskInfo() override;
    CLASS_DELETE_COPY_MOVE(TaskInfo)

    TaskManager *taskManager() const;

    QString name() const { return typeToString(type()); }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool runOnStatup() const { return m_runOnStatup; }
    void setRunOnStatup(bool runOnStatup);

    bool aborted() const { return m_aborted; }

    bool running() const { return m_running; }
    void setRunning(bool running);

    int intervalHours() const { return m_intervalHours; }
    void setIntervalHours(int intervalHours);

    QString title() const;
    static QString title(TaskType type);

    TaskInfo::TaskType type() const { return m_type; }
    void setType(TaskInfo::TaskType type);

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QDateTime lastRun() const { return m_lastRun; }
    void setLastRun(const QDateTime &lastRun);

    QDateTime plannedRun() const;

    QDateTime lastSuccess() const { return m_lastSuccess; }
    void setLastSuccess(const QDateTime &lastSuccess);

    virtual QByteArray data() const { return QByteArray(); }
    virtual void setData(const QByteArray &data) { Q_UNUSED(data); }

    TaskWorker *taskWorker() const { return m_taskWorker; }
    void setTaskWorker(TaskWorker *taskWorker);

    void editFromVariant(const QVariant &v);

    static QString typeToString(TaskInfo::TaskType type);
    static TaskInfo::TaskType stringToType(const QString &name);

signals:
    void enabledChanged();
    void runOnStatupChanged();
    void runningChanged();
    void intervalHoursChanged();
    void typeChanged();
    void lastRunChanged();
    void lastSuccessChanged();
    void taskWorkerChanged();

    void workStarted();
    void workFinished(bool success);

public slots:
    void run();
    void abortTask();

    virtual bool processResult(bool success) = 0;

protected slots:
    virtual void setupTaskWorker();
    virtual void runTaskWorker();

    virtual void handleFinished(bool success);

private:
    TaskWorker *createWorker();

private:
    bool m_enabled : 1 = false;
    bool m_runOnStatup : 1 = false;
    bool m_running : 1 = false;
    bool m_aborted : 1 = false; // transient

    quint16 m_intervalHours = 24;

    TaskType m_type = TypeNone;

    qint64 m_id = 0;

    QDateTime m_lastRun;
    QDateTime m_lastSuccess;

    TaskWorker *m_taskWorker = nullptr;
};

#endif // TASKINFO_H
