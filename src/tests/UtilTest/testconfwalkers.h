#pragma once

#include <conf/app.h>
#include <util/conf/confappswalker.h>

class TestApps : public ConfAppsWalker
{
public:
    explicit TestApps(const QList<App> &apps) : m_apps(apps) { }

    bool walkApps(const std::function<walkAppsCallback> &func) const override
    {
        for (App app : m_apps) {
            if (!func(app))
                return false;
        }
        return true;
    }

private:
    QList<App> m_apps;
};

inline App wildcardApp(const QString &pathsText, bool blocked = false)
{
    App app;
    app.isWildcard = true;
    app.blocked = blocked;
    app.appOriginPath = pathsText;
    return app;
}
