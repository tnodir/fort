#include "appalertconnswindow.h"

#include <QHeaderView>

#include <conf/confmanager.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <user/iniuser.h>
#include <util/window/widgetwindowstatewatcher.h>

using namespace Fort;

AppAlertConnsWindow::AppAlertConnsWindow(const App &app, QWidget *parent) :
    AppConnsWindow(app, parent, Qt::Window)
{
    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());

    // Not held by the Window Manager: save the state on close
    connect(this, &WidgetWindow::aboutToClose, this, [&] { saveWindowState(/*wasVisible=*/true); });
}

void AppAlertConnsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setProgAlertConnWindowGeometry(stateWatcher()->geometry());
    iniUser.setProgAlertConnWindowMaximized(stateWatcher()->maximized());

    auto header = connListView()->horizontalHeader();
    iniUser.setProgAlertConnListHeader(header->saveState());
    iniUser.setProgAlertConnListHeaderVersion(connListHeaderVersion);

    confManager()->saveIniUser();
}

void AppAlertConnsWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    // Next to the alert window, if there is no saved geometry
    AppConnsWindow::restoreWindowState();

    stateWatcher()->restore(this, size(), iniUser.progAlertConnWindowGeometry(),
            iniUser.progAlertConnWindowMaximized());

    if (iniUser.progAlertConnListHeaderVersion() == connListHeaderVersion) {
        auto header = connListView()->horizontalHeader();
        header->restoreState(iniUser.progAlertConnListHeader());

        // By the program's path
        updateProgramColumnHidden();
    }
}
