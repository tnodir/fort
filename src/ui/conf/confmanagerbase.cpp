#include "confmanagerbase.h"

#include <fortglobal.h>

#include "confmanager.h"

ConfManagerBase::ConfManagerBase(QObject *parent) : QObject(parent) { }

SqliteDb *ConfManagerBase::sqliteDb() const
{
    return Fort::confManager()->sqliteDb();
}
