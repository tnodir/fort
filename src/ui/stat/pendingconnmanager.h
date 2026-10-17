#ifndef PENDINGCONNMANAGER_H
#define PENDINGCONNMANAGER_H

#include <QTimer>
#include <QVector>

#include "statconnmanager.h"

class PendingConnManager : public StatConnManager
{
    Q_OBJECT

public:
    explicit PendingConnManager(
            const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(PendingConnManager)

    /* The user's decision of the pending connection */
    enum ConnDecision : qint8 {
        ConnBlock = 0,
        ConnAllow, // once, the outbound connection only
    };

    void setUp() override;

    bool logConn(const LogEntryConn &entry, bool urgent = false) override;

    virtual bool decideConn(qint64 connId, ConnDecision decision);

    /* The connections logged till the time are timed out */
    static qint64 expireTime();

signals:
    void timedOutConnsDeleted(const QVector<quint64> &flowIds);

protected:
    void setupWorker() override;
    void setupConfManager() override { }

    /* The pending connections are logged always: the Alert Program window shows them */
    bool canLogConn(const LogEntryConn & /*entry*/) const override { return true; }
    LogConnJob *createLogConnJob(const LogEntryConn &entry) const override;

    const char *migrationsSqlDir() const override;

    void checkClearConnOnStartup() override;
    void checkClearConnOnExit() override;

private:
    void removeConns(const QVector<qint64> &connIds);

    void deleteTimedOutConns();
    void blockTimedOutConns(const QVector<quint64> &flowIds);

private:
    qint64 m_lastConnTime = 0;

    QTimer m_timer;
};

#endif // PENDINGCONNMANAGER_H
