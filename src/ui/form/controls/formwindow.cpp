#include "formwindow.h"

#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

using namespace Fort;

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

void FormWindow::setupFormWindow(IniUser &iniUser, const QString &iniGroup)
{
    setupWindowIcon(iniUser);

    setupStateWatcher();
    setupWindowCapture(iniUser, iniGroup);
}

void FormWindow::setupWindowIcon(IniUser &iniUser)
{
    const auto refreshWindowIcon = [&](const QString &iconPath) {
        this->setWindowIcon(GuiUtil::overlayIcon(iconPath, windowOverlayIconPath()));
    };

    QString iconPath;

    if (iniUser.updateWindowIcons()) {
        auto trayIcon = windowManager()->trayIcon();

        iconPath = trayIcon->iconPath();

        connect(trayIcon, &TrayIcon::iconPathChanged, this, refreshWindowIcon);
    } else {
        iconPath = ":/icons/fort.png";
    }

    refreshWindowIcon(iconPath);

    QMetaObject::invokeMethod(
            this, [&] { windowManager()->taskbarButton().setWindowBadge(windowHandle()); },
            Qt::QueuedConnection);
}

void FormWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);

    restoreWindowState();

    ensureWindowScreenBounds();
}

void FormWindow::setupWindowCapture(IniUser &iniUser, const QString &iniGroup)
{
    if (iniUser.valueBool(iniGroup + "/excludeFromCapture", iniUser.excludeFromCapture())) {
        setExcludeFromCapture(true);
    }
}
