#ifndef TASKINFO_H
#define TASKINFO_H

#include <QDateTime>
#include <QObject>

#include "../util/classhelpers.h"

class FortManager;
class TaskManager;
class TaskWorker;

class TaskInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(
            int intervalHours READ intervalHours WRITE setIntervalHours NOTIFY intervalHoursChanged)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(TaskInfo::TaskType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QDateTime lastRun READ lastRun WRITE setLastRun NOTIFY lastRunChanged)
    Q_PROPERTY(
            QDateTime lastSuccess READ lastSuccess WRITE setLastSuccess NOTIFY lastSuccessChanged)
    Q_PROPERTY(bool running READ running NOTIFY taskWorkerChanged)

public:
    enum TaskType : qint16 { TypeNone = -1, UpdateChecker = 0, ZoneDownloader };
    Q_ENUM(TaskType)

    explicit TaskInfo(TaskInfo::TaskType type, TaskManager &taskManager);
    ~TaskInfo() override;
    CLASS_DELETE_COPY_MOVE(TaskInfo)

    TaskManager *taskManager() const;
    FortManager *fortManager() const;

    QString name() const { return typeToString(type()); }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool aborted() const { return m_aborted; }

    bool running() const { return m_running; }
    void setRunning(bool running);

    int intervalHours() const { return m_intervalHours; }
    void setIntervalHours(int intervalHours);

    QString title() const;

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

    QVariant editToVariant() const;
    void editFromVariant(const QVariant &v);

    static QString typeToString(TaskInfo::TaskType type);
    static TaskInfo::TaskType stringToType(const QString &name);

signals:
    void enabledChanged();
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
    void abort();

    virtual bool processResult(bool success) = 0;

protected slots:
    virtual void setupTaskWorker();
    virtual void runTaskWorker();

    virtual void handleFinished(bool success);

private:
    TaskWorker *createWorker();

private:
    bool m_enabled : 1;
    bool m_running : 1;
    bool m_aborted : 1; // transient

    quint16 m_intervalHours = 24;

    TaskType m_type = TypeNone;

    qint64 m_id = 0;

    QDateTime m_lastRun;
    QDateTime m_lastSuccess;

    TaskWorker *m_taskWorker = nullptr;
};

#endif // TASKINFO_H
