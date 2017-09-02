#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 ip READ ip WRITE setIp NOTIFY ipChanged)
    Q_PROPERTY(quint32 pid READ pid WRITE setPid NOTIFY pidChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)

public:
    explicit LogEntry(quint32 ip = 0, quint32 pid = 0,
                      const QString &path = QString(),
                      QObject *parent = nullptr);

    quint32 ip() const { return m_ip; }
    void setIp(quint32 ip);

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString path() const { return m_path; }
    void setPath(const QString &path);

signals:
    void ipChanged();
    void pidChanged();
    void pathChanged();

public slots:

private:
    friend class LogBuffer;

private:
    quint32 m_ip;
    quint32 m_pid;
    QString m_path;
};

#endif // LOGENTRY_H
