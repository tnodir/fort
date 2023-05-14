#include "homemainpage.h"

#include <QIcon>
#include <QLabel>
#include <QStackedLayout>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "aboutpage.h"
#include "homepage.h"

HomeMainPage::HomeMainPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void HomeMainPage::onRetranslateUi()
{
    m_btHome->setText(tr("My Fort"));
    m_btAbout->setText(tr("About"));
}

void HomeMainPage::setupUi()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(0);

    // Side Bar
    auto sideBar = setupSideBar();

    // Pages
    setupStackedLayout();

    layout->addLayout(sideBar);
    layout->addLayout(m_stackedLayout, 1);

    this->setLayout(layout);
}

QLayout *HomeMainPage::setupSideBar()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    setupSideBarButtons();

    layout->addWidget(m_btHome);
    layout->addWidget(m_btAbout);
    layout->addStretch();

    return layout;
}

void HomeMainPage::setupSideBarButtons()
{
    m_btHome = ControlUtil::createSideButton(":/icons/tower.png", [&] { setCurrentIndex(0); });
    m_btHome->setChecked(true);

    m_btAbout =
            ControlUtil::createSideButton(":/icons/information.png", [&] { setCurrentIndex(1); });
}

void HomeMainPage::setupStackedLayout()
{
    m_stackedLayout = new QStackedLayout();

    m_stackedLayout->addWidget(new HomePage(ctrl()));
    m_stackedLayout->addWidget(new AboutPage(ctrl()));
}

void HomeMainPage::setCurrentIndex(int tabIndex)
{
    m_stackedLayout->setCurrentIndex(tabIndex);
}
