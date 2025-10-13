#include "fortglobal.h"

#include <appinfo/appinfocache.h>
#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/confrulemanager.h>
#include <conf/confzonemanager.h>
#include <conf/firewallconf.h>
#include <driver/drivermanager.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <hostinfo/hostinfocache.h>
#include <log/logmanager.h>
#include <manager/autoupdatemanager.h>
#include <manager/envmanager.h>
#include <manager/hotkeymanager.h>
#include <manager/serviceinfomanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <stat/statconnmanager.h>
#include <stat/statmanager.h>
#include <task/taskmanager.h>
#include <user/usersettings.h>
#include <util/ioc/ioccontainer.h>

namespace Fort {

AppInfoCache *appInfoCache()
{
    return IoC<AppInfoCache>();
}

AutoUpdateManager *autoUpdateManager()
{
    return IoC<AutoUpdateManager>();
}

ConfAppManager *confAppManager()
{
    return IoC<ConfAppManager>();
}

ConfManager *confManager()
{
    return IoC<ConfManager>();
}

ConfRuleManager *confRuleManager()
{
    return IoC<ConfRuleManager>();
}

ConfZoneManager *confZoneManager()
{
    return IoC<ConfZoneManager>();
}

DriverManager *driverManager()
{
    return IoC<DriverManager>();
}

EnvManager *envManager()
{
    return IoC<EnvManager>();
}

FirewallConf *conf()
{
    return confManager()->conf();
}

FortManager *fortManager()
{
    return IoC<FortManager>();
}

FortSettings *settings()
{
    return IoC<FortSettings>();
}

UserSettings *userSettings()
{
    return IoC<UserSettings>();
}

HostInfoCache *hostInfoCache()
{
    return IoC<HostInfoCache>();
}

HotKeyManager *hotKeyManager()
{
    return IoC<HotKeyManager>();
}

IniOptions &ini()
{
    return settings()->iniOpt();
}

IniUser &iniUser()
{
    return userSettings()->iniUser();
}

LogManager *logManager()
{
    return IoC<LogManager>();
}

ServiceInfoManager *serviceInfoManager()
{
    return IoC<ServiceInfoManager>();
}

StatManager *statManager()
{
    return IoC<StatManager>();
}

StatConnManager *statConnManager()
{
    return IoC<StatConnManager>();
}

TaskManager *taskManager()
{
    return IoC<TaskManager>();
}

TranslationManager *translationManager()
{
    return IoC<TranslationManager>();
}

WindowManager *windowManager()
{
    return IoC<WindowManager>();
}

ZoneListModel *zoneListModel()
{
    return IoC<ZoneListModel>();
}

} // namespace Fort
