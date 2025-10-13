#ifndef FORTGLOBAL_H
#define FORTGLOBAL_H

#include <QObject>

class AppInfoCache;
class AutoUpdateManager;
class ConfAppManager;
class ConfManager;
class ConfRuleManager;
class ConfZoneManager;
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
class ServiceInfoManager;
class StatConnManager;
class StatManager;
class TaskManager;
class TranslationManager;
class UserSettings;
class WindowManager;
class ZoneListModel;

namespace Fort {

AppInfoCache *appInfoCache();
AutoUpdateManager *autoUpdateManager();
ConfAppManager *confAppManager();
ConfManager *confManager();
ConfRuleManager *confRuleManager();
ConfZoneManager *confZoneManager();
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
ServiceInfoManager *serviceInfoManager();
StatManager *statManager();
StatConnManager *statConnManager();
TaskManager *taskManager();
TranslationManager *translationManager();
UserSettings *userSettings();
WindowManager *windowManager();
ZoneListModel *zoneListModel();

}

#endif // FORTGLOBAL_H
