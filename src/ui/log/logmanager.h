#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(AppListModel)
QT_FORWARD_DECLARE_CLASS(AppStatModel)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(DriverWorker)
QT_FORWARD_DECLARE_CLASS(LogBuffer)
QT_FORWARD_DECLARE_CLASS(LogEntry)

class LogManager : public QObject
{
    Q_OBJECT

public:
    explicit LogManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    DriverWorker *driverWorker() const;
    AppListModel *appListModel() const;
    AppStatModel *appStatModel() const;

    void setActive(bool active);

    QString errorMessage() const { return m_errorMessage; }

    void initialize();

signals:
    void activeChanged();
    void errorMessageChanged();

public slots:
    void close();

private slots:
    void processLogBuffer(LogBuffer *logBuffer, bool success, quint32 errorCode);

private:
    void setErrorMessage(const QString &errorMessage);

    qint64 currentUnixTime() const;
    void setCurrentUnixTime(qint64 unixTime);

    void readLogAsync();
    void cancelAsyncIo();

    LogBuffer *getFreeBuffer();
    void addFreeBuffer(LogBuffer *logBuffer);

    void readLogEntries(LogBuffer *logBuffer);

private:
    bool m_active = false;

    FortManager *m_fortManager = nullptr;

    QList<LogBuffer *> m_freeBuffers;

    QString m_errorMessage;

    qint64 m_currentUnixTime = 0;
};

#endif // LOGMANAGER_H
