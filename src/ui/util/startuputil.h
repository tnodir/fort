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

    static bool installService();
    static bool uninstallService();

    static bool isServiceInstalled();
    static bool startService();

    static StartupMode getStartupMode();
    static void setStartupMode(int mode);

    static bool isServiceMode(int mode);
};

#endif // STARTUPUTIL_H
