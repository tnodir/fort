#ifndef HOSTINFO_H
#define HOSTINFO_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(HostInfoWorker)

class HostInfo : public QObject
{
    Q_OBJECT

public:
    explicit HostInfo(QObject *parent = nullptr);
    ~HostInfo() override;

signals:
    void lookupFinished(const QString &address, const QString &hostName);

public slots:
    void lookupHost(const QString &address);
    void clear();

private:
    void setupWorker();

    void abort();

private:
    HostInfoWorker *m_worker;
};

#endif // HOSTINFO_H
