#include "statmainpage.h"

#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../../../fortsettings.h"
#include "../../../util/iconcache.h"
#include "../../controls/controlutil.h"
#include "../statisticscontroller.h"
#include "connectionspage.h"
#include "trafficpage.h"

StatMainPage::StatMainPage(StatisticsController *ctrl, QWidget *parent) : StatBasePage(ctrl, parent)
{
    setupUi();
}

void StatMainPage::onRetranslateUi()
{
    m_tabBar->setTabText(0, tr("Traffic"));
    m_tabBar->setTabText(1, tr("Blocked Connections"));
}

void StatMainPage::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Main Tab Bar
    setupTabBar();
    layout->addWidget(m_tabBar);

    this->setLayout(layout);
}

void StatMainPage::setupTabBar()
{
    auto statisticsPage = new TrafficPage(ctrl());
    auto connectionsPage = new ConnectionsPage(ctrl());

    m_tabBar = new QTabWidget();
    m_tabBar->addTab(statisticsPage, IconCache::icon(":/icons/chart-bar.png"), QString());
    m_tabBar->addTab(connectionsPage, IconCache::icon(":/icons/connect.png"), QString());
}
