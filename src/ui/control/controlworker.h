#ifndef CONTROLWORKER_H
#define CONTROLWORKER_H

#include <QMutex>
#include <QObject>
#include <QRunnable>

QT_FORWARD_DECLARE_CLASS(QSharedMemory)
QT_FORWARD_DECLARE_CLASS(QSystemSemaphore)

class ControlWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ControlWorker(QSystemSemaphore *semaphore,
                           QSharedMemory *sharedMemory,
                           QObject *parent = nullptr);

    void run() override;

    bool post(const QString &command,
              const QStringList &args);

signals:
    void requestReady(const QString &command,
                      const QStringList &args);

public slots:
    void abort();

private:
    void processRequest();

    bool writeData(const QByteArray &data);
    QByteArray readData() const;

    bool writeDataStream(const QString &command,
                         const QStringList &args);
    bool readDataStream(QString &command,
                        QStringList &args) const;

private:
    volatile bool m_aborted;

    QSystemSemaphore *m_semaphore;
    QSharedMemory *m_sharedMemory;

    QMutex m_mutex;
};

#endif // CONTROLWORKER_H
