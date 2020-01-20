#ifndef TEST_H
#define TEST_H

#include <QObject>

class SqliteDb;

class Test : public QObject
{
    Q_OBJECT

private slots:
    void dbWriteRead();
    void monthStart();

private:
    void debugProcNew(SqliteDb *sqliteDb);

    void debugStatTraf(SqliteDb *sqliteDb);
    void debugStatTrafStep(SqliteDb *sqliteDb, const char *name,
                           const char *sql);
};

#endif // TEST_H
