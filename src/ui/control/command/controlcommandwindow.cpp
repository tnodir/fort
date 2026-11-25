#include "controlcommandwindow.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

#include "controlcommandutil.h"
#include "windowshow.h"

using namespace Fort;

namespace {

QStringList optionsTabNames()
{
    // Sync with enum OptMainPage::TabIndex
    return { "options", "interface", "addresses", "dns", "statistics", "schedule" };
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

bool checkShowPassword(ProcessCommandResult &r, const WindowShow &windowShow)
{
    // The Main window is shown without the password
    if (windowShow.code == WindowMain)
        return true;

    return ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0);
}

bool showWindow(ProcessCommandResult &r, const QVariantList &args)
{
    const QString name = args.value(1).toString();

    const WindowShow *windowShow = WindowShow::byName(name);
    if (!windowShow) {
        r.errorMessage = "Invalid <name>: " + name;
        return false;
    }

    if (!checkShowPassword(r, *windowShow))
        return false;

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
        r.errorMessage = "Usage: window show <name> [<tab>]; <name>: " + WindowShow::namesText()
                + "; <tab> of options: " + optionsTabNames().join('|') + " or its index";
        return false;
    }

    return showWindow(r, p.args);
}
