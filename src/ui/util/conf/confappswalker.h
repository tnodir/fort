#ifndef CONFAPPSWALKER_H
#define CONFAPPSWALKER_H

#include <QObject>

#include <functional>

#include <conf/app.h>

using walkAppsCallback = bool(const App &app);

class ConfAppsWalker
{
public:
    virtual bool walkApps(const std::function<walkAppsCallback> &func) = 0;
};

#endif // CONFAPPSWALKER_H
