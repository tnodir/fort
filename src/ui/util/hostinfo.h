#ifndef HOSTINFO_H
#define HOSTINFO_H

#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

class HostInfo : public QThread
{
    Q_OBJECT

public:
    explicit HostInfo(QObject *parent = nullptr);
    virtual ~HostInfo();

signals:
    void lookupFinished(const QString &address, const QString &hostName);

public slots:
    void lookupHost(const QString &address);
    void clear();

protected:
    void run() override;

private:
    void cancel();

    QString dequeueAddress();

private:
    volatile bool m_cancelled;

    QQueue<QString> m_queue;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // HOSTINFO_H
