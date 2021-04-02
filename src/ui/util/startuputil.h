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

    static bool isServiceInstalled();
    static bool installService();
    static bool uninstallService();

    static StartupMode getStartupMode();
    static void setStartupMode(int mode);

    static bool isServiceMode(int mode);
};

#endif // STARTUPUTIL_H
