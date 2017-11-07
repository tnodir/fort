#ifndef HOSTINFO_H
#define HOSTINFO_H

#include <QObject>

class HostInfoWorker;

class HostInfo : public QObject
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

private:
    void setupWorker();

    void cancel();

private:
    HostInfoWorker *m_worker;
};

#endif // HOSTINFO_H
