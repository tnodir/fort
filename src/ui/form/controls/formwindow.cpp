#include "formwindow.h"

#include <QLayout>

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

void FormWindow::shrinkToContents()
{
    auto *formLayout = layout();
    if (!formLayout)
        return;

    formLayout->invalidate();
    formLayout->activate();

    resize(width(), minimumSizeHint().height());
}

void FormWindow::shrinkToContentsLater()
{
    // A reused window keeps its height after its content shrinks: the layouts update the minimum
    // size only after the window is shown
    QMetaObject::invokeMethod(this, &FormWindow::shrinkToContents, Qt::QueuedConnection);
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
