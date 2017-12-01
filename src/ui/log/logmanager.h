#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

class AppBlockedModel;
class DatabaseManager;
class DriverWorker;
class LogBuffer;
class LogEntry;

class LogManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AppBlockedModel *appBlockedModel READ appBlockedModel CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit LogManager(DatabaseManager *databaseManager,
                        DriverWorker *driverWorker,
                        QObject *parent = nullptr);

    AppBlockedModel *appBlockedModel() const { return m_appBlockedModel; }

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

public slots:

    void clearModels() const;

    void setLogReadingEnabled(bool enabled);

private slots:
    void processLogBuffer(LogBuffer *logBuffer, bool success,
                          const QString &errorMessage);

private:
    void setErrorMessage(const QString &errorMessage);

    void setupDatabaseManager();
    void setupDriverWorker();

    void readLogAsync(LogBuffer *logBuffer);
    void cancelAsyncIo();

    LogBuffer *getFreeBuffer();

    void readLogEntries(LogBuffer *logBuffer);

private:
    bool m_logReadingEnabled;

    DatabaseManager *m_databaseManager;

    DriverWorker *m_driverWorker;
    QList<LogBuffer *> m_freeBuffers;

    AppBlockedModel *m_appBlockedModel;

    QString m_errorMessage;
};

#endif // LOGMANAGER_H
