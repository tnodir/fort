#include "menubutton.h"

#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>

#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

using namespace Fort;

namespace {

TrayIcon *trayIcon()
{
    return windowManager()->trayIcon();
}

}

MenuButton::MenuButton(QWidget *parent) : PushButton(parent)
{
    setupUi();
}

void MenuButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/large_tiles.png"));

    setMenu(trayIcon()->menu());
}

void MenuButton::mousePressEvent(QMouseEvent *e)
{
    const bool isLeftRightButton =
            (e->button() == Qt::LeftButton || e->button() == Qt::RightButton);

    if (isLeftRightButton && e->modifiers() == Qt::NoModifier) {
        PushButton::mousePressEvent(e);
    } else {
        setMousePressed(true);
    }
}

void MenuButton::mouseReleaseEvent(QMouseEvent *e)
{
    const bool mousePressed = this->mousePressed();

    PushButton::mouseReleaseEvent(e);

    if (!mousePressed)
        return;

    if (menu()->isVisible())
        return; // standard behavior

    trayIcon()->processMouseClick(e->button(), e->modifiers());
}
