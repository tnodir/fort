#include "appalertconnswindow.h"

#include <user/iniuser.h>

AppAlertConnsWindow::AppAlertConnsWindow(const App &app, QWidget *parent) :
    AppConnsWindow(app, parent, Qt::Window)
{
    setupAlertWindow();
}

void AppAlertConnsWindow::saveWindowState(bool /*wasVisible*/)
{
    saveIniWindowState(IniUser::progAlertConnWindowGroup());
}

void AppAlertConnsWindow::restoreWindowState()
{
    restoreAlertWindowState(IniUser::progAlertConnWindowGroup());
}
