#ifndef CONFAPPSWALKER_H
#define CONFAPPSWALKER_H

#include <QObject>

#include <functional>

class App;

using walkAppsCallback = bool(App &app);

class ConfAppsWalker
{
public:
    virtual bool walkApps(const std::function<walkAppsCallback> &func) const = 0;
};

#endif // CONFAPPSWALKER_H
