#ifndef STATMANAGER_H
#define STATMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "../util/classhelpers.h"

class FirewallConf;
class LogEntryBlockedIp;
class QuotaManager;
class SqliteDb;
class SqliteStmt;

class StatManager : public QObject
{
    Q_OBJECT

public:
    explicit StatManager(const QString &filePath, QuotaManager *quotaManager,
            QObject *parent = nullptr, quint32 openFlags = 0);
    ~StatManager() override;
    CLASS_DELETE_COPY_MOVE(StatManager)

    qint64 connBlockIdMin() const { return m_connBlockIdMin; }
    qint64 connBlockIdMax() const { return m_connBlockIdMax; }

    qint64 connTrafIdMin() const { return m_connTrafIdMin; }
    qint64 connTrafIdMax() const { return m_connTrafIdMax; }

    QuotaManager *quotaManager() const { return m_quotaManager; }

    const FirewallConf *conf() const { return m_conf; }
    virtual void setConf(const FirewallConf *conf);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    bool initialize();

    bool logProcNew(quint32 pid, const QString &appPath, qint64 unixTime = 0);
    bool logStatTraf(quint16 procCount, const quint32 *procTrafBytes, qint64 unixTime);

    bool logBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime);

    void getStatAppList(QStringList &list, QVector<qint64> &appIds);

    void deleteStatApp(qint64 appId);

    bool deleteOldConnBlock();
    bool deleteConn(qint64 rowIdTo, bool blocked);
    void deleteConnAll();

    void resetAppTrafTotals();
    bool hasAppTraf(qint64 appId);

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

    void setupConnBlockId();

    void setupByConf();

    void setupActivePeriod();
    void updateActivePeriod();

    void setupQuota();
    void clearQuotas(bool isNewDay, bool isNewMonth);
    void checkQuotas(quint32 inBytes);

    bool updateTrafDay(qint64 unixTime);

    void clearStmts();

    void logClear();
    void logClearApp(quint32 pid);

    void addCachedAppId(const QString &appPath, qint64 appId);
    qint64 getCachedAppId(const QString &appPath) const;
    void clearCachedAppId(const QString &appPath);
    void clearAppIdCache();

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0, bool blocked = false);
    bool deleteAppId(qint64 appId);

    void deleteOldTraffic(qint32 trafHour);

    void logTrafBytes(const QStmtList &insertStmtList, const QStmtList &updateStmtList,
            quint32 &sumInBytes, quint32 &sumOutBytes, quint32 pidFlag, quint32 inBytes,
            quint32 outBytes, qint64 unixTime);

    void updateTrafficList(const QStmtList &insertStmtList, const QStmtList &updateStmtList,
            quint32 inBytes, quint32 outBytes, qint64 appId = 0);

    bool updateTraffic(SqliteStmt *stmt, quint32 inBytes, quint32 outBytes, qint64 appId = 0);

    qint64 insertConn(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId);
    qint64 insertConnBlock(qint64 connId, quint8 blockReason);

    bool createConnBlock(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId);
    void deleteRangeConnBlock(qint64 rowIdFrom, qint64 rowIdTo);

    void deleteAppStmtList(const QStmtList &stmtList, SqliteStmt *stmtAppList);

    void doStmtList(const QStmtList &stmtList);

    SqliteStmt *getTrafficStmt(const char *sql, qint32 trafTime);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);
    SqliteStmt *getId2Stmt(const char *sql, qint64 id1, qint64 id2);

    SqliteStmt *getSqliteStmt(const char *sql);

private:
    bool m_isActivePeriodSet : 1;
    bool m_isActivePeriod : 1;

    quint8 m_activePeriodFromHour = 0;
    quint8 m_activePeriodFromMinute = 0;
    quint8 m_activePeriodToHour = 0;
    quint8 m_activePeriodToMinute = 0;

    qint32 m_trafHour = 0;
    qint32 m_trafDay = 0;
    qint32 m_trafMonth = 0;
    qint32 m_tick = 0;

    qint64 m_connBlockIdMin = 0;
    qint64 m_connBlockIdMax = 0;

    qint64 m_connTrafIdMin = 0;
    qint64 m_connTrafIdMax = 0;

    QuotaManager *m_quotaManager = nullptr;
    const FirewallConf *m_conf = nullptr;

    SqliteDb *m_sqliteDb = nullptr;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QHash<quint32, QString> m_appPidPathMap; // pid -> appPath
    QHash<QString, qint64> m_appPathIdCache; // appPath -> appId
};

#endif // STATMANAGER_H
