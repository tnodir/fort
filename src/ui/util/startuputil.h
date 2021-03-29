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

    static StartupMode getStartupMode();
    static void setStartupMode(StartupMode mode);
};

#endif // STARTUPUTIL_H
