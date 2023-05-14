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
#include "updatespage.h"

namespace {

QToolButton *createToolButton(const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = ControlUtil::createFlatToolButton(iconPath, onClicked);
    c->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    c->setAutoExclusive(true);
    c->setCheckable(true);

    c->setIconSize(QSize(24, 24));
    c->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    return c;
}

}

HomeMainPage::HomeMainPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void HomeMainPage::onRetranslateUi()
{
    m_btUpdates->setText(tr("Updates"));
    m_btAbout->setText(tr("About"));
}

void HomeMainPage::setupUi()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Side Bar
    auto sideBar = setupSideBar();

    // Pages
    setupStackedLayout();

    layout->addLayout(sideBar);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addLayout(m_stackedLayout, 1);

    this->setLayout(layout);
}

QLayout *HomeMainPage::setupSideBar()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    setupSideBarButtons();

    layout->addWidget(m_btUpdates);
    layout->addWidget(m_btAbout);
    layout->addStretch();

    return layout;
}

void HomeMainPage::setupSideBarButtons()
{
    QPalette palette;
    palette.setColor(QPalette::Highlight, QColor(0x26, 0x26, 0x26));

    m_btUpdates = createToolButton(":/icons/arrow_refresh_small.png", [&] { setCurrentIndex(0); });
    m_btUpdates->setPalette(palette);
    m_btUpdates->setChecked(true);

    m_btAbout = createToolButton(":/icons/information.png", [&] { setCurrentIndex(1); });
    m_btAbout->setPalette(palette);
}

void HomeMainPage::setupStackedLayout()
{
    m_stackedLayout = new QStackedLayout();

    m_stackedLayout->addWidget(new UpdatesPage(ctrl()));
    m_stackedLayout->addWidget(new AboutPage(ctrl()));
}

void HomeMainPage::setCurrentIndex(int tabIndex)
{
    m_stackedLayout->setCurrentIndex(tabIndex);
}
