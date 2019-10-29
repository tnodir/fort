#ifndef STATMANAGER_H
#define STATMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(QuotaManager)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)

class StatManager : public QObject
{
    Q_OBJECT

public:
    explicit StatManager(const QString &filePath,
                         QuotaManager *quotaManager,
                         QObject *parent = nullptr);
    ~StatManager() override;
    CLASS_DELETE_COPY_MOVE(StatManager)

    const FirewallConf *firewallConf() const { return m_conf; }
    void setFirewallConf(const FirewallConf *conf);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    bool initialize();

    void logProcNew(quint32 pid, const QString &appPath);
    void logStatTraf(quint16 procCount, const quint32 *procTrafBytes);

    void getAppList(QStringList &list, QVector<qint64> &appIds);

    void deleteApp(qint64 appId);

    void resetAppTotals();

    qint32 getTrafficTime(const char *sql, qint64 appId = 0);

    void getTraffic(const char *sql, qint32 trafTime,
                    qint64 &inBytes, qint64 &outBytes,
                    qint64 appId = 0);

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

    void replaceAppPathAt(int index, const QString &appPath);
    void replaceAppIdAt(int index, qint64 appId);
    void clearAppId(qint64 appId);
    void clearAppIds();

    void logClear();
    void logClearApp(quint32 pid, int index);

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);

    void updateTrafficList(const QStmtList &insertStmtList,
                           const QStmtList &updateStmtList,
                           quint32 inBytes, quint32 outBytes,
                           qint64 appId = 0);

    bool updateTraffic(SqliteStmt *stmt, quint32 inBytes,
                       quint32 outBytes, qint64 appId = 0);

    void stepStmtList(const QStmtList &stmtList);

    SqliteStmt *getTrafficStmt(const char *sql, qint32 trafTime);
    SqliteStmt *getAppStmt(const char *sql, qint64 appId);

    SqliteStmt *getSqliteStmt(const char *sql);

private:
    bool m_isActivePeriodSet    : 1;
    bool m_isActivePeriod       : 1;

    quint8 activePeriodFromHour;
    quint8 activePeriodFromMinute;
    quint8 activePeriodToHour;
    quint8 activePeriodToMinute;

    qint16 m_appFreeIndex;

    qint32 m_lastTrafHour;
    qint32 m_lastTrafDay;
    qint32 m_lastTrafMonth;

    qint32 m_lastTick;

    QuotaManager *m_quotaManager;
    const FirewallConf *m_conf;

    SqliteDb *m_sqliteDb;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QVector<qint16> m_appFreeIndexes;
    QHash<quint32, int> m_appIndexes;
    QStringList m_appPaths;
    QVector<qint64> m_appIds;
};

#endif // STATMANAGER_H
