#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

class AppListModel;
class AppStatModel;
class ConnListModel;
class FortManager;
class DriverWorker;
class LogBuffer;
class LogEntry;

class LogManager : public QObject
{
    Q_OBJECT

public:
    explicit LogManager(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    DriverWorker *driverWorker() const;
    AppListModel *appListModel() const;
    AppStatModel *appStatModel() const;
    ConnListModel *connListModel() const;

    virtual void setActive(bool active);

    QString errorMessage() const { return m_errorMessage; }

    virtual void initialize();

signals:
    void activeChanged();
    void errorMessageChanged();

public slots:
    virtual void close();

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
