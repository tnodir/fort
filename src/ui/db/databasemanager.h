#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVector>

class SqliteDb;
class SqliteStmt;

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString &filePath,
                             QObject *parent = nullptr);
    virtual ~DatabaseManager();

    bool initialize();

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    void addApp(const QString &appPath, bool &isNew);
    void addTraffic(quint16 procCount, const quint8 *procBits,
                    const quint32 *trafBytes);

    void getAppList(QStringList &list);

signals:

public slots:

private:
    bool createTables();

    qint64 getAppId(const QString &appPath, bool &isNew);

    SqliteStmt *getSqliteStmt(const char *sql);

    void insertTraffic(SqliteStmt *stmt, qint64 appId = 0);
    void updateTraffic(SqliteStmt *stmt, quint32 inBytes,
                          quint32 outBytes, qint64 appId = 0);

    static qint32 getUnixDay(qint64 unixTime);
    static qint32 getUnixMonth(qint64 unixTime);

private:
    qint32 m_lastUnixHour;
    qint32 m_lastUnixDay;
    qint32 m_lastUnixMonth;

    QString m_filePath;

    SqliteDb *m_sqliteDb;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QVector<qint64> m_appIds;
};

#endif // DATABASEMANAGER_H
