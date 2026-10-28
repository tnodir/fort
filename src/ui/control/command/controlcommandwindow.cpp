#include "controlcommandwindow.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

struct WindowShow
{
    const char *name;
    bool (*show)();
    bool (*showTab)(int index); // of the window with tabs
};

// As by the "home show" and "graph show": the home window exposed, the graph not activated
inline constexpr WindowShow windowShows[] = {
    { "home", [] { return windowManager()->exposeHomeWindow(); } },
    { "programs", [] { return windowManager()->showProgramsWindow(); } },
    { "options", [] { return windowManager()->showOptionsWindow(); },
            [](int index) { return windowManager()->showOptionsWindowTab(index); } },
    { "rules", [] { return windowManager()->showRulesWindow(); } },
    { "zones", [] { return windowManager()->showZonesWindow(); } },
    { "groups", [] { return windowManager()->showGroupsWindow(); } },
    { "speed-limits", [] { return windowManager()->showSpeedLimitsWindow(); } },
    { "time-periods", [] { return windowManager()->showTimePeriodsWindow(); } },
    { "connections", [] { return windowManager()->showConnectionsWindow(); } },
    { "statistics", [] { return windowManager()->showTrafficWindow(); } },
    { "services", [] { return windowManager()->showServicesWindow(); } },
    { "filter-sim", [] { return windowManager()->showFilterSimWindow(); } },
    { "graph", [] { return windowManager()->showGraphWindow(); } },
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
    if (!windowShow.showTab) {
        r.errorMessage = "The window has no tabs";
        return false;
    }

    // Of the Options window only
    const int tabIndex = optionsTabIndex(tabText);
    if (tabIndex < 0) {
        r.errorMessage = "Invalid <tab>: " + tabText;
        return false;
    }

    return windowShow.showTab(tabIndex);
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

    return windowShow->show();
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
