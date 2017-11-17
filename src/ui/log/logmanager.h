#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QAbstractItemModel>
#include <QObject>

class AppBlockedModel;
class DriverWorker;
class LogBuffer;
class LogEntry;

class LogManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit LogManager(DriverWorker *driverWorker,
                        QObject *parent = nullptr);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

public slots:
    QAbstractItemModel *appBlockedModel() const;

    void setLogReadingEnabled(bool enabled);

private slots:
    void processLogBuffer(LogBuffer *logBuffer, bool success,
                          const QString &errorMessage);

private:
    void setErrorMessage(const QString &errorMessage);

    void setupDriverWorker();

    void readLogAsync(LogBuffer *logBuffer);
    void cancelAsyncIo();

    LogBuffer *getFreeBuffer();

    void readLogEntries(LogBuffer *logBuffer);

private:
    bool m_logReadingEnabled;

    DriverWorker *m_driverWorker;
    QList<LogBuffer *> m_freeBuffers;

    AppBlockedModel *m_appBlockedModel;

    QString m_errorMessage;
};

#endif // LOGMANAGER_H
