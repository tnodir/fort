#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

class DriverWorker;
class LogBuffer;

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
    void setLogReadingEnabled(bool enabled);

private slots:
    void processLogBuffer(LogBuffer *logBuffer, bool success,
                          const QString &errorMessage);

private:
    void setErrorMessage(const QString &errorMessage);

    void setupDriverWorker();

    void readLogAsync(LogBuffer *logBuffer);
    void cancelAsyncIo();

private:
    bool m_logReadingEnabled;

    DriverWorker *m_driverWorker;
    LogBuffer *m_logBuffer;

    QString m_errorMessage;
};

#endif // LOGMANAGER_H
