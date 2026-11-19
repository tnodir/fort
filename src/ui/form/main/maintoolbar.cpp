#include "maintoolbar.h"

#include <QMenu>
#include <QToolButton>

#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

MainToolBar::MainToolBar(QWidget *parent) : QToolBar(parent)
{
    setupUi();
}

void MainToolBar::setupUi()
{
    const auto trayIcon = windowManager()->trayIcon();

    setMovable(false);
    setIconSize(QSize(16, 16));

    addMenu(trayIcon->blockTrafficMenu());
    addMenu(trayIcon->filterModeMenu());
}

void MainToolBar::addMenu(QMenu *menu)
{
    // The menu's icon and tool tip are of its selected action
    const auto a = menu->menuAction();

    addAction(a);

    auto button = qobject_cast<QToolButton *>(widgetForAction(a));
    button->setPopupMode(QToolButton::InstantPopup);
}
