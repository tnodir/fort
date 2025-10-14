#include "optionsbutton.h"

#include <QMenu>

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

OptionsButton::OptionsButton(int tabIndex, QWidget *parent) :
    ToolButton(parent), m_tabIndex(tabIndex)
{
    setupUi();

    connect(this, &QToolButton::clicked, this, &OptionsButton::showOptionsWindow);
}

void OptionsButton::showOptionsWindow()
{
    windowManager()->showOptionsWindowTab(m_tabIndex);
}

void OptionsButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/cog.png"));

    setPopupMode(QToolButton::MenuButtonPopup);
    setMenu(trayIcon()->optionsMenu());
}
