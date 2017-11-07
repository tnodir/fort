#ifndef HOSTINFOWORKER_H
#define HOSTINFOWORKER_H

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QRunnable>
#include <QWaitCondition>

class HostInfoWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit HostInfoWorker(QObject *parent = nullptr);

    void run() override;

signals:
    void lookupFinished(const QString &address, const QString &hostName);

public slots:
    void lookupHost(const QString &address);
    void clear();
    void cancel();

private:
    QString dequeueAddress();

private:
    volatile bool m_cancelled;

    QQueue<QString> m_queue;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // HOSTINFOWORKER_H
