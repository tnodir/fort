#include "fortglobal.h"

#include <conf/confmanager.h>
#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

namespace Fort {

ConfManager *confManager()
{
    return IoC<ConfManager>();
}

FirewallConf *conf()
{
    return confManager()->conf();
}

FortSettings *settings()
{
    return IoC<FortSettings>();
}

IniOptions &iniOpt()
{
    return settings()->iniOpt();
}

} // namespace Fort
