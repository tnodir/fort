#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(AppBlockedModel)
QT_FORWARD_DECLARE_CLASS(AppStatModel)
QT_FORWARD_DECLARE_CLASS(DatabaseManager)
QT_FORWARD_DECLARE_CLASS(DriverWorker)
QT_FORWARD_DECLARE_CLASS(LogBuffer)
QT_FORWARD_DECLARE_CLASS(LogEntry)

class LogManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AppBlockedModel *appBlockedModel READ appBlockedModel CONSTANT)
    Q_PROPERTY(AppStatModel *appStatModel READ appStatModel CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit LogManager(DatabaseManager *databaseManager,
                        DriverWorker *driverWorker,
                        QObject *parent = nullptr);

    AppBlockedModel *appBlockedModel() const { return m_appBlockedModel; }
    AppStatModel *appStatModel() const { return m_appStatModel; }

    QString errorMessage() const { return m_errorMessage; }

    void initialize();

signals:
    void errorMessageChanged();

public slots:
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
    AppStatModel *m_appStatModel;

    QString m_errorMessage;
};

#endif // LOGMANAGER_H
