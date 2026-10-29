#include "controlcommandwindow.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

struct WindowShow
{
    bool activate : 1 = true;
    bool expose : 1 = false;
    WindowCode code;
    const char *name;
};

// As by the "home show" and "graph show": the home window exposed, the graph not activated
inline constexpr WindowShow windowShows[] = {
    { .expose = true, .code = WindowHome, .name = "home" },
    { .code = WindowPrograms, .name = "programs" },
    { .code = WindowOptions, .name = "options" },
    { .code = WindowRules, .name = "rules" },
    { .code = WindowZones, .name = "zones" },
    { .code = WindowGroups, .name = "groups" },
    { .code = WindowSpeedLimits, .name = "speed-limits" },
    { .code = WindowTimePeriods, .name = "time-periods" },
    { .code = WindowConnections, .name = "connections" },
    { .code = WindowTraffic, .name = "statistics" },
    { .code = WindowServices, .name = "services" },
    { .code = WindowFilterSim, .name = "filter-sim" },
    { .activate = false, .code = WindowGraph, .name = "graph" },
};

const WindowShow *windowShowByName(const QString &name)
{
    for (const auto &windowShow : windowShows) {
        if (name == QLatin1String(windowShow.name))
            return &windowShow;
    }

    return nullptr;
}

QString windowNamesText()
{
    QStringList names;

    for (const auto &windowShow : windowShows) {
        names.append(windowShow.name);
    }

    return names.join('|');
}

QStringList optionsTabNames()
{
    // Sync with OptMainPage's tabs
    return { "options", "interface", "addresses", "statistics", "graph", "schedule" };
}

// The tab's index by its name or index, -1 when invalid
int optionsTabIndex(const QString &text)
{
    const QStringList names = optionsTabNames();

    bool ok = false;
    const int index = text.toInt(&ok);
    if (!ok)
        return names.indexOf(text);

    return (index < names.size()) ? index : -1;
}

bool showWindowTab(ProcessCommandResult &r, const WindowShow &windowShow, const QString &tabText)
{
    if (windowShow.code != WindowOptions) {
        r.errorMessage = "The window has no tabs";
        return false;
    }

    const int tabIndex = optionsTabIndex(tabText);
    if (tabIndex < 0) {
        r.errorMessage = "Invalid <tab>: " + tabText;
        return false;
    }

    return windowManager()->showOptionsWindowTab(tabIndex);
}

bool showWindow(ProcessCommandResult &r, const QVariantList &args)
{
    const QString name = args.value(1).toString();

    const WindowShow *windowShow = windowShowByName(name);
    if (!windowShow) {
        r.errorMessage = "Invalid <name>: " + name;
        return false;
    }

    if (args.size() > 2)
        return showWindowTab(r, *windowShow, args[2].toString());

    return windowManager()->showWindowByCode(
            windowShow->code, windowShow->activate, windowShow->expose);
}

}

bool ControlCommandWindow::processCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    if (p.args.value(0).toString() != "show" || p.args.size() < 2) {
        r.errorMessage = "Usage: window show <name> [<tab>]; <name>: " + windowNamesText()
                + "; <tab> of options: " + optionsTabNames().join('|') + " or its index";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return showWindow(r, p.args);
}
