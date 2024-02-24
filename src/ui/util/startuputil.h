#ifndef STARTUPUTIL_H
#define STARTUPUTIL_H

#include <QObject>

class StartupUtil
{
public:
    enum AutoRunMode : qint8 { StartupDisabled = 0, StartupCurrentUser, StartupAllUsers };

    static const wchar_t *serviceName();

    static bool isServiceInstalled();
    static void setServiceInstalled(bool install);

    static bool isServiceRunning();

    static bool startService();
    static bool stopService();

    static AutoRunMode autoRunMode();
    static void setAutoRunMode(int mode, const QString &defaultLanguage = QString());

    static bool isExplorerIntegrated();
    static void setExplorerIntegrated(bool integrate);

    static void clearGlobalExplorerIntegrated();

    static QString registryPasswordHash();
    static void setRegistryPasswordHash(const QString &passwordHash);

    static void setPortable(bool portable);
};

#endif // STARTUPUTIL_H
