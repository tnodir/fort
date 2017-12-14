#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString &filePath,
                             QObject *parent = nullptr);
    virtual ~DatabaseManager();

    const FirewallConf *firewallConf() const { return m_conf; }
    void setFirewallConf(const FirewallConf *conf);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    bool initialize();

    qint64 logProcNew(const QString &appPath, bool &isNew);
    void logStatTraf(quint16 procCount, const quint8 *procBits,
                     const quint32 *trafBytes);
    void logClear();

    void getAppList(QStringList &list, QVector<qint64> &appIds);

    qint64 getAppId(const QString &appPath);

    void deleteApp(qint64 appId);

    void resetAppTotals();

    qint32 getTrafficTime(const char *sql, qint64 appId = 0);

    void getTraffic(const char *sql, qint32 trafTime,
                    qint64 &inBytes, qint64 &outBytes,
                    qint64 appId = 0);

signals:

public slots:
    void clear();

private:
    typedef QList<SqliteStmt *> QStmtList;

    bool createTables();

    void clearStmts();

    qint64 createAppId(const QString &appPath);

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
    qint32 m_lastTrafHour;
    qint32 m_lastTrafDay;
    qint32 m_lastTrafMonth;

    QString m_filePath;

    const FirewallConf *m_conf;

    SqliteDb *m_sqliteDb;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QStringList m_appPaths;
    QVector<qint64> m_appIds;
};

#endif // DATABASEMANAGER_H
