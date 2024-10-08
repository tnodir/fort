#include "menubutton.h"

#include <QGuiApplication>
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
    const auto button = e->button();
    const auto modifiers = e->modifiers();

    if (button == Qt::LeftButton && modifiers == Qt::NoModifier) {
        QPushButton::mousePressEvent(e);
    }
}

void MenuButton::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);

    const auto button = e->button();
    const auto modifiers = e->modifiers();

    if (button == Qt::LeftButton && modifiers == Qt::NoModifier)
        return; // standard behavior

    trayIcon()->processMouseClick(button, modifiers);
}
