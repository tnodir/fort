#ifndef TASKINFO_H
#define TASKINFO_H

#include <QDateTime>
#include <QObject>

#include <util/classhelpers.h>

class TaskManager;
class TaskWorker;

constexpr int TaskDefaultIntervalHours = 24;

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
    void setEnabled(bool v) { m_enabled = v; }

    bool runOnStartup() const { return m_runOnStartup; }
    void setRunOnStartup(bool v) { m_runOnStartup = v; }

    bool delayStartup() const { return m_delayStartup; }
    void setDelayStartup(bool v) { m_delayStartup = v; }

    bool aborted() const { return m_aborted; }

    bool running() const { return m_running; }
    void setRunning(bool v) { m_running = v; }

    QString title() const;
    static QString title(TaskType type);

    TaskInfo::TaskType type() const { return m_type; }
    void setType(TaskInfo::TaskType v) { m_type = v; }

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    int maxRetries() const { return m_maxRetries; }
    void setMaxRetries(int v) { m_maxRetries = quint8(v); }

    int retrySeconds() const { return m_retrySeconds; }
    void setRetrySeconds(int v) { m_retrySeconds = quint16(v); }

    int intervalHours() const { return m_intervalHours; }
    void setIntervalHours(int v) { m_intervalHours = quint16(v); }

    QDateTime lastRun() const { return m_lastRun; }
    void setLastRun(const QDateTime &v) { m_lastRun = v; }

    QDateTime lastSuccess() const { return m_lastSuccess; }
    void setLastSuccess(const QDateTime &v) { m_lastSuccess = v; }

    virtual QByteArray data() const { return QByteArray(); }
    virtual void setData(const QByteArray &data) { Q_UNUSED(data); }

    TaskWorker *taskWorker() const { return m_taskWorker; }
    void setTaskWorker(TaskWorker *v) { m_taskWorker = v; }

    void editFromVariant(const QVariant &v);

    qint64 secondsToRun(const QDateTime &now, bool isFirstRun) const;

    virtual void initialize() { }

    static QString typeToString(TaskInfo::TaskType type);
    static TaskInfo::TaskType stringToType(const QString &name);

signals:
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
    bool m_runOnStartup : 1 = false;
    bool m_delayStartup : 1 = false;
    bool m_running : 1 = false;
    bool m_aborted : 1 = false; // transient

    TaskType m_type = TypeNone;

    quint8 m_failedCount = 0; // transient
    quint8 m_maxRetries = 0;
    quint16 m_retrySeconds = 0;
    quint16 m_intervalHours = TaskDefaultIntervalHours;

    qint64 m_id = 0;

    QDateTime m_lastRun;
    QDateTime m_lastSuccess;

    TaskWorker *m_taskWorker = nullptr;
};

#endif // TASKINFO_H
