#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(AppListModel)
QT_FORWARD_DECLARE_CLASS(AppStatModel)
QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(StatManager)
QT_FORWARD_DECLARE_CLASS(DriverWorker)
QT_FORWARD_DECLARE_CLASS(LogBuffer)
QT_FORWARD_DECLARE_CLASS(LogEntry)

class LogManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AppListModel *appListModel READ appListModel CONSTANT)
    Q_PROPERTY(AppStatModel *appStatModel READ appStatModel CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit LogManager(ConfManager *confManager,
                        StatManager *statManager,
                        DriverWorker *driverWorker,
                        QObject *parent = nullptr);

    AppListModel *appListModel() const { return m_appListModel; }
    AppStatModel *appStatModel() const { return m_appStatModel; }

    void setActive(bool active);

    QString errorMessage() const { return m_errorMessage; }

    void initialize();

signals:
    void activeChanged();
    void errorMessageChanged();

public slots:
    void close();

private slots:
    void processLogBuffer(LogBuffer *logBuffer, bool success,
                          const QString &errorMessage);

private:
    void setErrorMessage(const QString &errorMessage);

    void readLogAsync();
    void cancelAsyncIo();

    LogBuffer *getFreeBuffer();
    void addFreeBuffer(LogBuffer *logBuffer);

    void readLogEntries(LogBuffer *logBuffer);

private:
    bool m_active = false;

    AppListModel *m_appListModel = nullptr;
    AppStatModel *m_appStatModel = nullptr;

    DriverWorker *m_driverWorker = nullptr;
    QList<LogBuffer *> m_freeBuffers;

    QString m_errorMessage;
};

#endif // LOGMANAGER_H
