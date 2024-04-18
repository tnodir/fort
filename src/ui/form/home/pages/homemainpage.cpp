#include "homemainpage.h"

#include <QIcon>
#include <QLabel>
#include <QStackedWidget>
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
    buttonAt(TabHome)->setText(tr("My Fort"));
    buttonAt(TabAbout)->setText(tr("About"));
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
    layout->addWidget(m_stackedPages, 1);

    this->setLayout(layout);
}

QLayout *HomeMainPage::setupSideBar()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    setupSideBarButtons();

    for (QToolButton *bt : m_buttons) {
        layout->addWidget(bt);
    }
    layout->addStretch();

    return layout;
}

void HomeMainPage::setupSideBarButtons()
{
    auto btHome =
            ControlUtil::createSideButton(":/icons/fort.png", [&] { setCurrentTab(TabHome); });
    btHome->setChecked(true);

    auto btAbout = ControlUtil::createSideButton(
            ":/icons/information.png", [&] { setCurrentTab(TabAbout); });

    m_buttons = { btHome, btAbout };
}

void HomeMainPage::setupStackedLayout()
{
    m_stackedPages = new QStackedWidget();

    m_stackedPages->addWidget(new HomePage(ctrl()));
    m_stackedPages->addWidget(new AboutPage(ctrl()));
}

void HomeMainPage::setCurrentTab(TabIndex tabIndex)
{
    if (m_stackedPages->currentIndex() == tabIndex)
        return;

    m_stackedPages->setCurrentIndex(tabIndex);

    buttonAt(tabIndex)->animateClick();
}
