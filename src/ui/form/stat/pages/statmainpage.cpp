#include "statmainpage.h"

#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QToolButton>
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

    m_btOptions->setText(tr("Options"));
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

    setupCornerWidget();
}

void StatMainPage::setupCornerWidget()
{
    auto layout = setupCornerLayout();

    auto w = new QWidget();
    w->setLayout(layout);

    m_tabWidget->setCornerWidget(w);
}

QLayout *StatMainPage::setupCornerLayout()
{
    // Options button
    m_btOptions = ControlUtil::createFlatToolButton(":/icons/cog.png");

    connect(m_btOptions, &QAbstractButton::clicked, windowManager(),
            &WindowManager::showStatOptionsWindow);

    // Menu button
    m_btMenu = windowManager()->createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btOptions, ControlUtil::createVSeparator(), m_btMenu });
    layout->setContentsMargins(0, 0, 0, 0);

    return layout;
}
