#ifndef FORTGLOBAL_H
#define FORTGLOBAL_H

#include <QObject>

class AppInfoCache;
class AppInfoManager;
class AskPendingManager;
class AutoUpdateManager;
class ConfAppManager;
class ConfManager;
class ConfRuleManager;
class ConfZoneManager;
class ControlManager;
class DriverManager;
class EnvManager;
class FirewallConf;
class FortManager;
class FortSettings;
class HostInfoCache;
class HotKeyManager;
class IniOptions;
class IniUser;
class LogManager;
class NativeEventFilter;
class QuotaManager;
class RpcManager;
class ServiceInfoManager;
class ServiceManager;
class StatConnManager;
class StatManager;
class TaskManager;
class TranslationManager;
class UserSettings;
class WindowManager;
class ZoneListModel;

namespace Fort {

template<class T>
T *dependency();

AppInfoCache *appInfoCache();
AppInfoManager *appInfoManager();
AskPendingManager *askPendingManager();
AutoUpdateManager *autoUpdateManager();
ConfAppManager *confAppManager();
ConfManager *confManager();
ConfRuleManager *confRuleManager();
ConfZoneManager *confZoneManager();
ControlManager *controlManager();
DriverManager *driverManager();
EnvManager *envManager();
FirewallConf *conf();
FortManager *fortManager();
FortSettings *settings();
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
StatManager *statManager();
StatConnManager *statConnManager();
TaskManager *taskManager();
TranslationManager *translationManager();
UserSettings *userSettings();
WindowManager *windowManager();
ZoneListModel *zoneListModel();

}

#endif // FORTGLOBAL_H
