#include "maintoolbar.h"

#include <QMenu>
#include <QToolButton>

#include <form/controls/actioncheckbox.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/installationbutton.h>
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

    addCheckBox(trayIcon->filterEnabledAction());
    addAction(trayIcon->snoozeAlertsAction());

    addMenu(trayIcon->blockTrafficMenu());
    addMenu(trayIcon->filterModeMenu());

    addSeparator();

    addWidget(new InstallationButton());

    m_headerButtons = new HeaderButtons();

    addWidget(m_headerButtons);
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
