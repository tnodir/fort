#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

QT_FORWARD_DECLARE_CLASS(ControlWorker)
QT_FORWARD_DECLARE_CLASS(FortManager)

class ControlManager : public QObject
{
    Q_OBJECT

public:
    explicit ControlManager(const QString &globalName,
                            const QString &scriptPath,
                            QObject *parent = nullptr);
    ~ControlManager() override;

    bool isClient() const { return m_isClient; }

    bool listen(FortManager *fortManager);
    bool post(const QStringList &args);

signals:

public slots:

private slots:
    void processRequest(const QString &scriptPath,
                        const QStringList &args);

private:
    void setupWorker();

    void abort();

private:
    bool m_isClient;

    QString m_scriptPath;

    FortManager *m_fortManager;
    ControlWorker *m_worker;

    QSystemSemaphore m_semaphore;
    QSharedMemory m_sharedMemory;
};

#endif // CONTROLMANAGER_H
