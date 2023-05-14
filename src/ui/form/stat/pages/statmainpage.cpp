#include "statmainpage.h"

#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/stat/statisticscontroller.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "connectionspage.h"
#include "trafficpage.h"

StatMainPage::StatMainPage(StatisticsController *ctrl, QWidget *parent) : StatBasePage(ctrl, parent)
{
    setupUi();
}

void StatMainPage::onRetranslateUi()
{
    m_tabWidget->setTabText(0, tr("Traffic"));
    m_tabWidget->setTabText(1, tr("Blocked Connections"));
}

void StatMainPage::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Main Tab Bar
    setupTabBar();
    layout->addWidget(m_tabWidget);

    this->setLayout(layout);
}

void StatMainPage::setupTabBar()
{
    auto statisticsPage = new TrafficPage(ctrl());
    auto connectionsPage = new ConnectionsPage(ctrl());

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(statisticsPage, IconCache::icon(":/icons/chart_bar.png"), QString());
    m_tabWidget->addTab(connectionsPage, IconCache::icon(":/icons/connect.png"), QString());

    // Menu button
    m_btMenu = ControlUtil::createLinkButton(":/icons/node-tree.png");
    m_btMenu->setMenu(windowManager()->trayIcon()->menu());

    m_tabWidget->setCornerWidget(m_btMenu);
}
