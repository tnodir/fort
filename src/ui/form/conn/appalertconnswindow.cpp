#include "appalertconnswindow.h"

#include <user/iniuser.h>

AppAlertConnsWindow::AppAlertConnsWindow(const App &app, QWidget *parent) :
    AppConnsWindow(app, parent, Qt::Window)
{
    hideClearAll();

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
