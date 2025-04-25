#include "confmanagerbase.h"

#include "confmanager.h"

ConfManagerBase::ConfManagerBase(QObject *parent) : QObject(parent) { }

SqliteDb *ConfManagerBase::sqliteDb() const
{
    return confManager()->sqliteDb();
}
