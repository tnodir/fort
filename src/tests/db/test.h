#ifndef TEST_H
#define TEST_H

#include <QObject>

class SqliteDb;

class Test : public QObject
{
    Q_OBJECT

private slots:
    void dbWriteRead();

private:
    void debugProcNew(SqliteDb *sqliteDb);
    void debugStatTraf(SqliteDb *sqliteDb);
};

#endif // TEST_H
