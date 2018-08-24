#ifndef CONTROLWORKER_H
#define CONTROLWORKER_H

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

    bool post(const QString &scriptPath,
              const QStringList &args);

signals:
    void requestReady(const QString &scriptPath,
                      const QStringList &args);

public slots:
    void abort();

private:
    void processRequest();

    bool writeData(const QByteArray &data);
    QByteArray readData() const;

    bool writeDataStream(const QString &scriptPath,
                         const QStringList &args);
    bool readDataStream(QString &scriptPath,
                        QStringList &args) const;

private:
    volatile bool m_aborted;

    QSystemSemaphore *m_semaphore;
    QSharedMemory *m_sharedMemory;
};

#endif // CONTROLWORKER_H
