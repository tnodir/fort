#include "sqliteengine.h"

#include "sqlite3.h"

void SqliteEngine::initialize()
{
    sqlite3_initialize();
}

void SqliteEngine::shutdown()
{
    sqlite3_shutdown();
}
