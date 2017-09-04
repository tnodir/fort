#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 ip READ ip WRITE setIp NOTIFY ipChanged)
    Q_PROPERTY(quint32 pid READ pid WRITE setPid NOTIFY pidChanged)
    Q_PROPERTY(QString dosPath READ dosPath WRITE setDosPath NOTIFY dosPathChanged)
    Q_PROPERTY(QString ipText READ ipText NOTIFY ipChanged)
    Q_PROPERTY(QString path READ path NOTIFY dosPathChanged)

public:
    explicit LogEntry(quint32 ip = 0, quint32 pid = 0,
                      const QString &dosPath = QString(),
                      QObject *parent = nullptr);

    quint32 ip() const { return m_ip; }
    void setIp(quint32 ip);

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString dosPath() const { return m_dosPath; }
    void setDosPath(const QString &dosPath);

    QString ipText() const;
    QString path() const;

signals:
    void ipChanged();
    void pidChanged();
    void dosPathChanged();

public slots:

private:
    quint32 m_ip;
    quint32 m_pid;
    QString m_dosPath;
};

#endif // LOGENTRY_H
