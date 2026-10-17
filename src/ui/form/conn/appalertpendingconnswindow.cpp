#include "appalertpendingconnswindow.h"

#include <user/iniuser.h>

AppAlertPendingConnsWindow::AppAlertPendingConnsWindow(const App &app, QWidget *parent) :
    AppPendingConnsWindow(app, parent, Qt::Window)
{
    setupAlertWindow();
}

void AppAlertPendingConnsWindow::saveWindowState(bool /*wasVisible*/)
{
    saveIniWindowState(IniUser::progAlertPendingConnWindowGroup());
}

void AppAlertPendingConnsWindow::restoreWindowState()
{
    restoreAlertWindowState(IniUser::progAlertPendingConnWindowGroup());
}
