#ifndef SQLITEDBEXT_H
#define SQLITEDBEXT_H

#include <QObject>

class SqliteDb;

class SqliteDbExt
{
public:
    static void registerExtensions(SqliteDb *sqliteDb);
};

#endif // SQLITEDBEXT_H
