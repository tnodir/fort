#include "statisticsbutton.h"

#include <QMenu>

#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

using namespace Fort;

StatisticsButton::StatisticsButton(QWidget *parent) : ToolButton(parent)
{
    setupUi();

    connect(this, &QToolButton::clicked, this, &StatisticsButton::showTrafficWindow);
}

void StatisticsButton::showTrafficWindow()
{
    windowManager()->showTrafficWindow();
}

void StatisticsButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/chart_bar.png"));

    setPopupMode(QToolButton::MenuButtonPopup);
    setMenu(windowManager()->trayIcon()->statisticsMenu());
}
