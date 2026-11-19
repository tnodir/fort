#include "formwindow.h"

#include <QAction>
#include <QLayout>

#include <form/controls/controlutil.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/windowmenubutton.h>
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
    m_helpAction = ControlUtil::setupHelpShortcut(this, [this] { return helpSection(); });
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
        setupWindowMenuButton();
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

void FormWindow::changeEvent(QEvent *event)
{
    WidgetWindow::changeEvent(event);

    if (event->type() == QEvent::ParentChange && isDockable()) {
        updateDocked();
    }
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

void FormWindow::setupWindowMenuButton()
{
    m_headerButtons = findChild<HeaderButtons *>();
    if (!m_headerButtons)
        return;

    auto btWindowMenu = m_headerButtons->windowMenuButton();
    btWindowMenu->setFormWindow(this);
    btWindowMenu->setVisible(true);
}

void FormWindow::updateDocked()
{
    const bool docked = isDocked();

    // The Main window's Help action shows the docked window's help
    m_helpAction->setEnabled(!docked);

    // The Main window's menu has them
    if (m_headerButtons) {
        m_headerButtons->setVisible(!docked);
    }

    updateDockedMinimumSize(docked);
}

void FormWindow::updateDockedMinimumSize(bool docked)
{
    // The dock host doesn't move a dock to a place where the docks' minimum sizes don't fit: a
    // docked window has only its contents' minimum size
    if (docked) {
        if (!m_windowMinimumSize.isValid()) {
            m_windowMinimumSize = minimumSize();
        }
        setMinimumSize(0, 0);
    } else if (m_windowMinimumSize.isValid()) {
        setMinimumSize(m_windowMinimumSize);
        m_windowMinimumSize = QSize();
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
