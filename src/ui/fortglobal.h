#ifndef FORTGLOBAL_H
#define FORTGLOBAL_H

#include <QObject>

#include <util/ioc/ioccontainer.h>

class AppInfoCache;
class AppInfoManager;
class AskPendingManager;
class AutoUpdateManager;
class ConfAppManager;
class ConfGroupManager;
class ConfManager;
class ConfRuleManager;
class ConfSpeedLimitManager;
class ConfTimePeriodManager;
class ConfZoneManager;
class ControlManager;
class DriverManager;
class EnvManager;
class FirewallConf;
class FortManager;
class FortSettings;
class GroupListModel;
class HostInfoCache;
class HotKeyManager;
class IniOptions;
class IniUser;
class LogManager;
class NativeEventFilter;
class QuotaManager;
class RpcManager;
class SpeedLimitListModel;
class ServiceInfoManager;
class ServiceManager;
class StatConnManager;
class StatManager;
class TaskManager;
class TimePeriodListModel;
class TranslationManager;
class UserSettings;
class WindowManager;
class ZoneListModel;

namespace Fort {

template<class T>
constexpr T *dependency()
{
    return IoCDependency<T>();
}

AppInfoCache *appInfoCache();
AppInfoManager *appInfoManager();
AskPendingManager *askPendingManager();
AutoUpdateManager *autoUpdateManager();
ConfAppManager *confAppManager();
ConfGroupManager *confGroupManager();
ConfManager *confManager();
ConfRuleManager *confRuleManager();
ConfSpeedLimitManager *confSpeedLimitManager();
ConfTimePeriodManager *confTimePeriodManager();
ConfZoneManager *confZoneManager();
ControlManager *controlManager();
DriverManager *driverManager();
EnvManager *envManager();
FirewallConf &conf();
FortManager *fortManager();
FortSettings *settings();
GroupListModel *groupListModel();
HostInfoCache *hostInfoCache();
HotKeyManager *hotKeyManager();
IniOptions &ini();
IniUser &iniUser();
LogManager *logManager();
NativeEventFilter *nativeEventFilter();
QuotaManager *quotaManager();
RpcManager *rpcManager();
ServiceInfoManager *serviceInfoManager();
ServiceManager *serviceManager();
SpeedLimitListModel *speedLimitListModel();
StatConnManager *statConnManager();
StatManager *statManager();
TaskManager *taskManager();
TimePeriodListModel *timePeriodListModel();
TranslationManager *translationManager();
UserSettings *userSettings();
WindowManager *windowManager();
ZoneListModel *zoneListModel();

}

#endif // FORTGLOBAL_H
