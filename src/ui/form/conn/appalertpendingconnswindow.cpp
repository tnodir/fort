#include "appalertpendingconnswindow.h"

#include <user/iniuser.h>

AppAlertPendingConnsWindow::AppAlertPendingConnsWindow(const App &app, QWidget *parent) :
    AppPendingConnsWindow(app, parent, Qt::Window)
{
    setupWindow();
}

void AppAlertPendingConnsWindow::restoreWindowState()
{
    restoreAlertWindowState();
}

QString AppAlertPendingConnsWindow::iniGroup() const
{
    return IniUser::progAlertPendingConnWindowGroup();
}
