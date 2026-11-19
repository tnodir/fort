#include "formwindow.h"

#include <QAction>
#include <QLayout>

#include <form/controls/controlutil.h>
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
    ControlUtil::setupHelpShortcut(this, [this] { return helpSection(); });
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

void FormWindow::setupFormWindow(IniUser &iniUser, const QString &captureIniGroup)
{
    setupWindowIcon(iniUser);

    if (isDockable()) {
        setupActionShortcuts();
    }

    setupStateWatcher();
    setupWindowCapture(iniUser, !captureIniGroup.isEmpty() ? captureIniGroup : iniGroup());
}

void FormWindow::saveIniWindowGeometry()
{
    auto &iniUser = Fort::iniUser();
    const auto group = iniGroup();

    iniUser.setWindowGeometry(group, stateWatcher()->geometry());
    iniUser.setWindowMaximized(group, stateWatcher()->maximized());
}

void FormWindow::restoreIniWindowGeometry(const QSize &defaultSize)
{
    const auto &iniUser = Fort::iniUser();
    const auto group = iniGroup();

    stateWatcher()->restore(
            this, defaultSize, iniUser.windowGeometry(group), iniUser.windowMaximized(group));
}

void FormWindow::setupWindowIcon(IniUser &iniUser)
{
    const auto refreshWindowIcon = [&](const QString &iconPath) {
        this->setWindowIcon(GuiUtil::overlayIcon(iconPath, windowIconPath()));
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

void FormWindow::setupActionShortcuts()
{
    // Each of the docked windows' same shortcuts works in its window only
    const auto actions = findChildren<QAction *>();
    for (QAction *a : actions) {
        if (a->shortcut().isEmpty())
            continue;

        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(a);
    }
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
