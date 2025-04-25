#ifndef SQLITEUTILBASE_H
#define SQLITEUTILBASE_H

#include <sqlite/sqlite_types.h>

class SqliteUtilBase
{
public:
    virtual SqliteDb *sqliteDb() const = 0;

    void removeDbFilesToCleanOpen() const;

protected:
    bool beginWriteTransaction();
    void commitTransaction();
    void endTransaction(bool &ok);
};

#endif // SQLITEUTILBASE_H
