#include "dbmanagerbase.h"

#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

#include <conf/confmanager.h>

ConfManager *DbManagerBase::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *DbManagerBase::conf() const
{
    return confManager()->conf();
}

FortSettings *DbManagerBase::settings() const
{
    return IoC<FortSettings>();
}

SqliteDb *DbManagerBase::sqliteDb() const
{
    return confManager()->sqliteDb();
}
