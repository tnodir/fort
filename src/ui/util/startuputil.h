#ifndef STARTUPUTIL_H
#define STARTUPUTIL_H

#include <QObject>

class StartupUtil
{
public:
    enum StartupMode {
        StartupDisabled = 0,
        StartupCurrentUser,
        StartupAllUsers,
        StartupAllUsersBackground
    };

    static const wchar_t *const serviceName;
    static const wchar_t *const serviceDisplay;

    static bool isServiceInstalled();
    static bool startService();

    static StartupMode getStartupMode();
    static void setStartupMode(int mode, const QString &defaultLanguage = QString());

    static bool isServiceMode(int mode);

    static bool isExplorerIntegrated();
    static void integrateExplorer(bool integrate);
};

#endif // STARTUPUTIL_H
