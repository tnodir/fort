#ifndef DNSNRPTCHECK_H
#define DNSNRPTCHECK_H

#include <QElapsedTimer>
#include <QObject>

#include <util/classhelpers.h>

class DnsQuery;
class QHostInfo;

// Whether the system resolver's queries reach the listener: its rule of the Name Resolution Policy
// Table may be overridden, e.g. by the Group Policy's ones, or the connections to the listener may
// be blocked, e.g. by a VPN's DNS leak protection
class DnsNrptCheck : public QObject
{
    Q_OBJECT

public:
    explicit DnsNrptCheck(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(DnsNrptCheck)

    bool isRunning() const { return m_lookupId >= 0; }

    // The last check was started within the time
    bool isStartedWithin(qint64 msecs) const
    {
        return m_startTimer.isValid() && !m_startTimer.hasExpired(msecs);
    }

    // Resolves a unique name by the system resolver
    void start();
    void abort();

    // Whether the query is of the check's name, received by the listener
    bool checkQuery(const DnsQuery &query);

signals:
    // The listener's receipt of the query is processed before the lookup's end
    void finished(bool received);

private:
    void onLookedUp(const QHostInfo &info);

private:
    bool m_received = false;

    int m_lookupId = -1;

    QElapsedTimer m_startTimer;

    QString m_name;
};

#endif // DNSNRPTCHECK_H
