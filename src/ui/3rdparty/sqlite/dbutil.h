#ifndef DBUTIL_H
#define DBUTIL_H

#include <QObject>
#include <QVariant>

class SqliteDb;
class SqliteStmt;

class DbUtil
{
public:
    explicit DbUtil(SqliteDb *sqliteDb, bool *ok = nullptr);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    DbUtil &sql(const QString &sql);
    DbUtil &sql(const char *sql);

    DbUtil &vars(const QVariantList &vars);
    DbUtil &vars(const QVariantHash &varsMap);

    bool prepare(SqliteStmt &stmt);
    bool prepareRow(SqliteStmt &stmt);

    QVariant execute(int resultCount, bool &ok);
    QVariant execute(int resultCount = 1);
    bool executeOk();

    int getFreeId(int minId, int maxId);

protected:
    void setResult(bool v);

private:
    QVariantList m_vars;
    QVariantHash m_varsMap;

    QByteArray m_sqlUtf8;
    const char *m_sql = nullptr;

    SqliteDb *m_sqliteDb = nullptr;
    bool *m_ok = nullptr;
};

#endif // DBUTIL_H
