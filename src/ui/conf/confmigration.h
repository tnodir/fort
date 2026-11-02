#ifndef CONFMIGRATION_H
#define CONFMIGRATION_H

#include <sqlite/sqlitedb.h>

class ConfMigration
{
public:
    static SqliteDb::MigrateOptions migrateOptions();
};

#endif // CONFMIGRATION_H
