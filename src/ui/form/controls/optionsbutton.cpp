#include "optionsbutton.h"

#include <QMenu>

#include <form/tray/trayicon.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

OptionsButton::OptionsButton(int tabIndex, QWidget *parent) :
    ToolButton(parent), m_tabIndex(tabIndex)
{
    setupUi();

    connect(this, &QToolButton::clicked, this, &OptionsButton::showOptionsWindow);
}

TrayIcon *OptionsButton::trayIcon() const
{
    return IoC<WindowManager>()->trayIcon();
}

void OptionsButton::showOptionsWindow()
{
    IoC<WindowManager>()->showOptionsWindowTab(m_tabIndex);
}

void OptionsButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/cog.png"));

    setPopupMode(QToolButton::MenuButtonPopup);
    setMenu(trayIcon()->optionsMenu());
}
