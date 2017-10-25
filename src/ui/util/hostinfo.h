#ifndef HOSTINFO_H
#define HOSTINFO_H

#include <QObject>
#include <QHash>

typedef QHash<int, QString> lookupids_map_t;

class HostInfo : public QObject
{
    Q_OBJECT
//    Q_PROPERTY(QString hostName READ hostName CONSTANT)

public:
    explicit HostInfo(QObject *parent = nullptr);
    virtual ~HostInfo();

//    QString hostName() const { return m_info.hostName(); }
//    QString errorString() const { return m_info.errorString(); }

signals:
    void hostLookedup(const QString &name, bool success);

public slots:
    void lookupHost(const QString &name);

    void abortHostLookups();

private slots:
//    void handleLookedupHost(const QHostInfo &info);

private:
    void abortHostLookup(int lookupId);

private:
//    QHostInfo m_info;

    lookupids_map_t m_lookupIds;
};

#endif // HOSTINFO_H
