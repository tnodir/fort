#include "maintoolbar.h"

#include <QMenu>
#include <QToolButton>

#include <form/controls/actioncheckbox.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/installationbutton.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

#include "mainwindow.h"

using namespace Fort;

MainToolBar::MainToolBar(MainWindow *mainWindow) : QToolBar(mainWindow)
{
    setupUi(mainWindow);
}

void MainToolBar::setupUi(MainWindow *mainWindow)
{
    const auto trayIcon = windowManager()->trayIcon();

    setMovable(false);
    setIconSize(QSize(16, 16));

    addCheckBox(trayIcon->filterEnabledAction());
    addAction(trayIcon->snoozeAlertsAction());

    addMenu(trayIcon->blockTrafficMenu());
    addMenu(trayIcon->filterModeMenu());

    addSeparator();

    addAction(mainWindow->lockAction());
    addAction(mainWindow->unlockAction());

    addWidget(new InstallationButton());

    auto headerButtons = new HeaderButtons();
    headerButtons->setSeparatorVisible(true);
    addWidget(headerButtons);
}

void MainToolBar::addCheckBox(QAction *action)
{
    auto c = new ActionCheckBox(action);

    // Its hot key works in the Main window
    c->addAction(action);

    // As the tool bar's buttons
    c->setFocusPolicy(Qt::NoFocus);

    addWidget(c);
}

void MainToolBar::addMenu(QMenu *menu)
{
    // The menu's icon and tool tip are of its selected action
    const auto a = menu->menuAction();

    addAction(a);

    auto button = qobject_cast<QToolButton *>(widgetForAction(a));
    button->setPopupMode(QToolButton::InstantPopup);
}
