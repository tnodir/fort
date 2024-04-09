#ifndef DBQUERY_H
#define DBQUERY_H

#include <QObject>
#include <QVariant>

class SqliteDb;
class SqliteStmt;

class DbQuery
{
public:
    explicit DbQuery(SqliteDb *sqliteDb, bool *ok = nullptr);

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    DbQuery &sql(const QString &sql);
    DbQuery &sql(const char *sql);

    DbQuery &vars(const QVariantList &vars);
    DbQuery &vars(const QVariantHash &varsMap);

    bool prepare(SqliteStmt &stmt);
    bool prepareRow(SqliteStmt &stmt);

    QVariant execute(int resultCount, bool &ok);
    QVariant execute(int resultCount = 1);
    bool executeOk();

    int getFreeId(int maxId, int minId = 1);

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

#endif // DBQUERY_H
