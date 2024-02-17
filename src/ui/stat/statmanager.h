#ifndef STATMANAGER_H
#define STATMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

class FirewallConf;
class IniOptions;
class LogEntryProcNew;
class LogEntryStatTraf;

class StatManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit StatManager(const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(StatManager)

    const FirewallConf *conf() const { return m_conf; }
    virtual void setConf(const FirewallConf *conf);

    const IniOptions *ini() const;

    SqliteDb *sqliteDb() const { return m_sqliteDb.data(); }

    void setUp() override;

    bool logProcNew(const LogEntryProcNew &entry, qint64 unixTime = 0);
    bool logStatTraf(const LogEntryStatTraf &entry, qint64 unixTime = 0);

    void getStatAppList(QStringList &list, QVector<qint64> &appIds);

    virtual bool deleteStatApp(qint64 appId);

    virtual bool resetAppTrafTotals();
    bool hasAppTraf(qint64 appId);

    qint32 getTrafficTime(const char *sql, qint64 appId = 0);

    void getTraffic(
            const char *sql, qint32 trafTime, qint64 &inBytes, qint64 &outBytes, qint64 appId = 0);

signals:
    void trafficCleared();

    void appStatRemoved(qint64 appId);
    void appCreated(qint64 appId, const QString &appPath);
    void trafficAdded(qint64 unixTime, quint32 inBytes, quint32 outBytes);

    void connChanged();

    void appTrafTotalsResetted();

public slots:
    virtual bool clearTraffic();

private:
    bool setupDb();

    void setupTrafDate();

    void setupByConf();

    void setupActivePeriod();
    void updateActivePeriod();

    void clearQuotas(bool isNewDay, bool isNewMonth);
    void checkQuotas(quint32 inBytes);

    bool updateTrafDay(qint64 unixTime);

    void logClear();
    void logClearApp(quint32 pid);

    void addCachedAppId(const QString &appPath, qint64 appId);
    qint64 getCachedAppId(const QString &appPath) const;
    void clearCachedAppId(const QString &appPath);
    void clearAppIdCache();

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0);
    bool deleteAppId(qint64 appId);

    void deleteOldTraffic(qint32 trafHour);

    void logTrafBytes(const SqliteStmtList &insertStmtList, const SqliteStmtList &updateStmtList,
            quint32 &sumInBytes, quint32 &sumOutBytes, quint32 pid, quint32 inBytes,
            quint32 outBytes, qint64 unixTime, bool logStat);

    void updateTrafficList(const SqliteStmtList &insertStmtList,
            const SqliteStmtList &updateStmtList, quint32 inBytes, quint32 outBytes,
            qint64 appId = 0);

    bool updateTraffic(SqliteStmt *stmt, quint32 inBytes, quint32 outBytes, qint64 appId = 0);

    SqliteStmt *getStmt(const char *sql);
    SqliteStmt *getTrafficStmt(const char *sql, qint32 trafTime);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);

    bool beginTransaction();
    void commitTransaction(bool &ok);

private:
    bool m_isActivePeriodSet : 1 = false;
    bool m_isActivePeriod : 1 = false;

    quint8 m_activePeriodFromHour = 0;
    quint8 m_activePeriodFromMinute = 0;
    quint8 m_activePeriodToHour = 0;
    quint8 m_activePeriodToMinute = 0;

    qint32 m_trafHour = 0;
    qint32 m_trafDay = 0;
    qint32 m_trafMonth = 0;
    qint32 m_tick = 0;

    const FirewallConf *m_conf = nullptr;

    SqliteDbPtr m_sqliteDb;

    QHash<quint32, QString> m_appPidPathMap; // pid -> appPath
    QHash<QString, qint64> m_appPathIdCache; // appPath -> appId
};

#endif // STATMANAGER_H
