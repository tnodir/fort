#include "menubutton.h"

#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>

#include <form/tray/trayicon.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

MenuButton::MenuButton(QWidget *parent) : QPushButton(parent)
{
    setupUi();
}

TrayIcon *MenuButton::trayIcon() const
{
    return IoC<WindowManager>()->trayIcon();
}

void MenuButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/large_tiles.png"));

    setMenu(trayIcon()->menu());
}

void MenuButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier) {
        QPushButton::mousePressEvent(e);
    }

    m_mousePressed = true;
}

void MenuButton::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);

    if (!m_mousePressed)
        return;

    m_mousePressed = false;

    if (menu()->isVisible())
        return; // standard behavior

    trayIcon()->processMouseClick(e->button(), e->modifiers());
}
