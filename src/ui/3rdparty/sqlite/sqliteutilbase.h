#ifndef SQLITEUTILBASE_H
#define SQLITEUTILBASE_H

#include <QVariant>

#include <sqlite/sqlite_types.h>

class SqliteUtilBase
{
public:
    virtual SqliteDb *sqliteDb() const = 0;

    void removeDbFilesToCleanOpen() const;

    bool backupDbFile(const QString &path) const;

protected:
    bool beginWriteTransaction();
    void commitTransaction();
    void endTransaction(bool &ok);

    bool executeWrite(const char *sql, const QVariantList &vars);
};

#endif // SQLITEUTILBASE_H
