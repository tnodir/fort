#include "appalertconnswindow.h"

#include <user/iniuser.h>

AppAlertConnsWindow::AppAlertConnsWindow(const App &app, QWidget *parent) :
    AppConnsWindow(app, parent, Qt::Window)
{
    setupWindow();
}

void AppAlertConnsWindow::restoreWindowState()
{
    restoreAlertWindowState();
}

QString AppAlertConnsWindow::iniGroup() const
{
    return IniUser::progAlertConnWindowGroup();
}
