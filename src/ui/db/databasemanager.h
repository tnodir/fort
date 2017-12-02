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

    void handleProcNew(const QString &path);
    void handleStatTraf(quint16 procCount, const quint8 *procBits,
                        const quint32 *trafBytes);

signals:

public slots:

private:
    bool createTables();

    qint64 getAppId(const QString &appPath);

    SqliteStmt *getSqliteStmt(const char *sql);

private:
    QString m_filePath;

    SqliteDb *m_sqliteDb;

    QHash<const char *, SqliteStmt *> m_sqliteStmts;

    QStringList m_appPaths;
    QVector<qint64> m_appIds;
};

#endif // DATABASEMANAGER_H
