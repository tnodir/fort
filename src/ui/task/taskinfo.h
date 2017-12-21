#ifndef TASKINFO_H
#define TASKINFO_H

#include <QDateTime>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(TaskWorker)

class TaskInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int intervalHours READ intervalHours WRITE setIntervalHours NOTIFY intervalHoursChanged)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(TaskInfo::TaskType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QDateTime lastRun READ lastRun WRITE setLastRun NOTIFY lastRunChanged)
    Q_PROPERTY(QDateTime lastSuccess READ lastSuccess WRITE setLastSuccess NOTIFY lastSuccessChanged)
    Q_PROPERTY(bool running READ running NOTIFY taskWorkerChanged)

public:
    enum TaskType {
        TypeNone = -1,
        UpdateChecker = 0,
        Tasix,
        Uzonline
    };
    Q_ENUM(TaskType)

    explicit TaskInfo(TaskInfo::TaskType type, QObject *parent = nullptr);
    virtual ~TaskInfo();

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    int intervalHours() const { return m_intervalHours; }
    void setIntervalHours(int intervalHours);

    QString title() const;

    TaskInfo::TaskType type() const { return m_type; }
    void setType(TaskInfo::TaskType type);

    QDateTime lastRun() const { return m_lastRun; }
    void setLastRun(const QDateTime &lastRun);

    QDateTime plannedRun() const;

    QDateTime lastSuccess() const { return m_lastSuccess; }
    void setLastSuccess(const QDateTime &lastSuccess);

    TaskWorker *taskWorker() const { return m_taskWorker; }
    void setTaskWorker(TaskWorker *taskWorker);

    bool running() const { return m_taskWorker != nullptr; }

    void rawData(QByteArray &data) const;
    void setRawData(const QByteArray &data);

    static QString typeToString(TaskInfo::TaskType type);
    static TaskInfo::TaskType stringToType(const QString &name);

    static QDateTime now();

signals:
    void enabledChanged();
    void intervalHoursChanged();
    void typeChanged();
    void lastRunChanged();
    void lastSuccessChanged();
    void taskWorkerChanged();

    void workFinished(bool success);

public slots:
    void run();
    void abort();

private slots:
    void handleFinished(bool success);

private:
    TaskWorker *createWorker();

private:
    uint m_enabled          : 1;
    uint m_aborted          : 1;  // transient
    uint m_intervalHours    : 16;

    TaskType m_type;

    QDateTime m_lastRun;
    QDateTime m_lastSuccess;

    TaskWorker *m_taskWorker;
};

#endif // TASKINFO_H
