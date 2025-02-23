#include "confmanagerbase.h"

#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

ConfManagerBase::ConfManagerBase(QObject *parent) : QObject(parent) { }

ConfManager *ConfManagerBase::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *ConfManagerBase::conf() const
{
    return confManager()->conf();
}

SqliteDb *ConfManagerBase::sqliteDb() const
{
    return confManager()->sqliteDb();
}
