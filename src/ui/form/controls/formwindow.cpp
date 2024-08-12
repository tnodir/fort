#include "formwindow.h"

#include <user/iniuser.h>
#include <util/window/widgetwindowstatewatcher.h>

FormWindow::FormWindow(QWidget *parent, Qt::WindowFlags f) :
    WidgetWindow(parent, f), m_stateWatcher(new WidgetWindowStateWatcher(this))
{
}

void FormWindow::setExcludeFromCapture(bool v)
{
    if (m_excludeFromCapture == v)
        return;

    m_excludeFromCapture = v;

    excludeWindowFromCapture(this, excludeFromCapture());
}

void FormWindow::setupFormWindow(IniUser *iniUser, const QString &iniGroup)
{
    setupStateWatcher();
    setupWindowCapture(iniUser, iniGroup);
}

void FormWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);

    restoreWindowState();
}

void FormWindow::setupWindowCapture(IniUser *iniUser, const QString &iniGroup)
{
    if (iniUser->valueBool(iniGroup + "/excludeFromCapture", iniUser->excludeFromCapture())) {
        setExcludeFromCapture(true);
    }
}
