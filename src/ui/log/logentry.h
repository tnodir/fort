#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 ip READ ip WRITE setIp NOTIFY ipChanged)
    Q_PROPERTY(quint32 pid READ pid WRITE setPid NOTIFY pidChanged)
    Q_PROPERTY(QString kernelPath READ kernelPath WRITE setKernelPath NOTIFY kernelPathChanged)

public:
    explicit LogEntry(quint32 ip = 0, quint32 pid = 0,
                      const QString &kernelPath = QString(),
                      QObject *parent = nullptr);

    quint32 ip() const { return m_ip; }
    void setIp(quint32 ip);

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath);

signals:
    void ipChanged();
    void pidChanged();
    void kernelPathChanged();

public slots:

private:
    quint32 m_ip;
    quint32 m_pid;
    QString m_kernelPath;
};

#endif // LOGENTRY_H
