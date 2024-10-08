#ifndef STARTUPUTIL_H
#define STARTUPUTIL_H

#include <QObject>

#include <util/service/service_types.h>

class StartupUtil
{
public:
    enum AutoRunMode : qint8 { StartupDisabled = 0, StartupCurrentUser, StartupAllUsers };

    static const wchar_t *serviceName();

    static bool isServiceInstalled();
    static void setServiceInstalled(bool install = true);

    static bool isServiceRunning();

    static bool startService();
    static bool stopService(ServiceControlCode controlCode = ServiceControlStop);

    static AutoRunMode autoRunMode();
    static void setAutoRunMode(int mode, const QString &defaultLanguage = QString());

    static bool isExplorerIntegrated();
    static void setExplorerIntegrated(bool integrate);

    static void clearGlobalExplorerIntegrated();

    static QString registryPasswordHash();
    static void setRegistryPasswordHash(const QString &passwordHash);

    static bool registryIsDriverAdmin();
    static void setRegistryIsDriverAdmin(bool isDriverAdmin);

    static void setPortable(bool portable);
};

#endif // STARTUPUTIL_H
