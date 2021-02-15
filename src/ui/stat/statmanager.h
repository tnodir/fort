#ifndef STATMANAGER_H
#define STATMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "../util/classhelpers.h"

class FirewallConf;
class QuotaManager;
class SqliteDb;
class SqliteStmt;

class StatManager : public QObject
{
    Q_OBJECT

public:
    explicit StatManager(
            const QString &filePath, QuotaManager *quotaManager, QObject *parent = nullptr);
    ~StatManager() override;
    CLASS_DELETE_COPY_MOVE(StatManager)

    const FirewallConf *firewallConf() const { return m_conf; }
    void setFirewallConf(const FirewallConf *conf);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    bool initialize();

    bool logProcNew(quint32 pid, const QString &appPath, qint64 unixTime = 0);
    bool logStatTraf(quint16 procCount, const quint32 *procTrafBytes, qint64 unixTime);

    bool logBlockedIp(bool inbound, quint8 blockReason, quint8 ipProto, quint16 localPort,
            quint16 remotePort, quint32 localIp, quint32 remoteIp, quint32 pid,
            const QString &appPath, qint64 unixTime);

    void getStatAppList(QStringList &list, QVector<qint64> &appIds);

    void deleteStatApp(qint64 appId, const QString &appPath);

    void resetAppTrafTotals();

    qint32 getTrafficTime(const char *sql, qint64 appId = 0);

    void getTraffic(
            const char *sql, qint32 trafTime, qint64 &inBytes, qint64 &outBytes, qint64 appId = 0);

signals:
    void appCreated(qint64 appId, const QString &appPath);

    void trafficAdded(qint64 unixTime, quint32 inBytes, quint32 outBytes);

public slots:
    void clear();

private:
    using QStmtList = QList<SqliteStmt *>;

    void initializeActivePeriod();
    void initializeQuota();

    void clearStmts();

    void logClear();
    void logClearApp(quint32 pid);

    void addCachedAppId(const QString &appPath, qint64 appId);
    qint64 getCachedAppId(const QString &appPath) const;
    void clearCachedAppId(const QString &appPath);
    void clearAppIdCache();

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime, bool blocked = false);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0, bool blocked = false);

    void updateTrafficList(const QStmtList &insertStmtList, const QStmtList &updateStmtList,
            quint32 inBytes, quint32 outBytes, qint64 appId = 0);

    bool updateTraffic(SqliteStmt *stmt, quint32 inBytes, quint32 outBytes, qint64 appId = 0);

    qint64 createConn(bool inbound, quint8 ipProto, quint16 localPort, quint16 remotePort,
            quint32 localIp, quint32 remoteIp, quint32 pid, qint64 unixTime, qint64 appId);
    bool createConnBlock(qint64 connId, quint8 blockReason);
    void removeOldConnBlock(int keepCount);

    void stepStmtList(const QStmtList &stmtList);

    SqliteStmt *getTrafficStmt(const char *sql, qint32 trafTime);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);

    SqliteStmt *getSqliteStmt(const char *sql);

private:
    bool m_isActivePeriodSet : 1;
    bool m_isActivePeriod : 1;

    quint8 activePeriodFromHour = 0;
    quint8 activePeriodFromMinute = 0;
    quint8 activePeriodToHour = 0;
    quint8 activePeriodToMinute = 0;

    qint32 m_lastTrafHour = 0;
    qint32 m_lastTrafDay = 0;
    qint32 m_lastTrafMonth = 0;
    qint32 m_lastTick = 0;

    int m_connBlockInc = 999999999; // to trigger on first check

    QuotaManager *m_quotaManager = nullptr;
    const FirewallConf *m_conf = nullptr;

    SqliteDb *m_sqliteDb = nullptr;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QHash<quint32, QString> m_appPidPathMap; // pid -> appPath
    QHash<QString, qint64> m_appPathIdCache; // appPath -> appId
};

#endif // STATMANAGER_H
