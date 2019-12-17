#include "optionswindow.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QTabBar>
#include <QVBoxLayout>

OptionsWindow::OptionsWindow(QWidget *parent) :
    WidgetWindow(parent)
{
    setupUi();
    retranslateUi();
}

void OptionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(2, 2, 2, 2);
    //layout->setMargin(0);

    m_tabBar = new QTabBar();
    m_tabBar->addTab(QIcon(":/images/cog.png"), QString());
    m_tabBar->addTab(QIcon(":/images/link.png"), QString());
    m_tabBar->addTab(QIcon(":/images/application_double.png"), QString());
    m_tabBar->addTab(QIcon(":/images/application.png"), QString());
    m_tabBar->addTab(QIcon(":/images/chart_line.png"), QString());
    m_tabBar->addTab(QIcon(":/images/clock.png"), QString());
    layout->addWidget(m_tabBar);

    this->setLayout(layout);
}

void OptionsWindow::retranslateUi()
{
    m_tabBar->setTabText(0, tr("Options"));
    m_tabBar->setTabText(1, tr("IPv4 Addresses"));
    m_tabBar->setTabText(2, tr("Program Groups"));
    m_tabBar->setTabText(3, tr("Programs"));
    m_tabBar->setTabText(4, tr("Statistics"));
    m_tabBar->setTabText(5, tr("Schedule"));
}
