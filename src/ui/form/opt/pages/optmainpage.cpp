#include "optmainpage.h"

#include <QIcon>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <fort_version.h>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>
#include <fortsettings.h>
#include <util/iconcache.h>

#include "addressespage.h"
#include "applicationspage.h"
#include "optionspage.h"
#include "schedulepage.h"
#include "statisticspage.h"

OptMainPage::OptMainPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void OptMainPage::onRetranslateUi()
{
    m_tabBar->setTabText(0, tr("Options"));
    m_tabBar->setTabText(1, tr("IP Addresses"));
    m_tabBar->setTabText(2, tr("Application Groups"));
    m_tabBar->setTabText(3, tr("Statistics"));
    m_tabBar->setTabText(4, tr("Schedule"));

    m_btLogs->setText(tr("Logs"));
    m_btProfile->setText(tr("Profile"));
    m_btStat->setText(tr("Statistics"));
    m_btReleases->setText(tr("Releases"));

    m_btOk->setText(tr("OK"));
    m_btApply->setText(tr("Apply"));
    m_btCancel->setText(tr("Cancel"));
}

void OptMainPage::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Main Tab Bar
    setupTabBar();
    layout->addWidget(m_tabBar);

    // Dialog butons
    auto buttonsLayout = setupDialogButtons();
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);
}

void OptMainPage::setupTabBar()
{
    auto optionsPage = new OptionsPage(ctrl());
    auto addressesPage = new AddressesPage(ctrl());
    auto applicationsPage = new ApplicationsPage(ctrl());
    auto statisticsPage = new StatisticsPage(ctrl());
    auto schedulePage = new SchedulePage(ctrl());

    m_pages = { optionsPage, addressesPage, applicationsPage, statisticsPage, schedulePage };

    m_tabBar = new QTabWidget();
    m_tabBar->addTab(ControlUtil::wrapToScrollArea(optionsPage), IconCache::icon(":/icons/cog.png"),
            QString());
    m_tabBar->addTab(addressesPage, IconCache::icon(":/icons/ip.png"), QString());
    m_tabBar->addTab(
            applicationsPage, IconCache::icon(":/icons/application_double.png"), QString());
    m_tabBar->addTab(ControlUtil::wrapToScrollArea(statisticsPage),
            IconCache::icon(":/icons/chart_bar.png"), QString());
    m_tabBar->addTab(schedulePage, IconCache::icon(":/icons/clock.png"), QString());

    connect(m_tabBar, &QTabWidget::currentChanged, this,
            [&](int tabIndex) { m_pages[tabIndex]->onPageActivated(); });
}

QLayout *OptMainPage::setupDialogButtons()
{
    auto buttonsLayout = new QHBoxLayout();

    m_btLogs = ControlUtil::createLinkButton(":/icons/folder.png", settings()->logsPath());
    m_btProfile = ControlUtil::createLinkButton(":/icons/folder.png", settings()->profilePath());
    m_btStat = ControlUtil::createLinkButton(":/icons/folder.png", settings()->statPath());
    m_btReleases = ControlUtil::createLinkButton(":/icons/github.png", APP_UPDATES_URL);

    connect(m_btLogs, &QAbstractButton::clicked, this, &OptMainPage::onLinkClicked);
    connect(m_btProfile, &QAbstractButton::clicked, this, &OptMainPage::onLinkClicked);
    connect(m_btStat, &QAbstractButton::clicked, this, &OptMainPage::onLinkClicked);
    connect(m_btReleases, &QAbstractButton::clicked, this, &OptMainPage::onLinkClicked);

    buttonsLayout->addWidget(m_btLogs);
    buttonsLayout->addWidget(m_btProfile);
    buttonsLayout->addWidget(m_btStat);
    buttonsLayout->addWidget(m_btReleases);

    buttonsLayout->addStretch();

    m_btOk = new QPushButton();
    m_btApply = new QPushButton();
    m_btCancel = new QPushButton();

    connect(m_btOk, &QAbstractButton::clicked, ctrl(), &OptionsController::saveChanges);
    connect(m_btApply, &QAbstractButton::clicked, ctrl(), &OptionsController::applyChanges);
    connect(m_btCancel, &QAbstractButton::clicked, ctrl(), &OptionsController::closeWindow);

    setupApplyCancelButtons();

    buttonsLayout->addWidget(m_btOk);
    buttonsLayout->addWidget(m_btApply);
    buttonsLayout->addWidget(m_btCancel);

    return buttonsLayout;
}

void OptMainPage::setupApplyCancelButtons()
{
    const auto refreshButtons = [&](bool anyEdited) {
        m_btApply->setEnabled(anyEdited);
        m_btCancel->setEnabled(anyEdited);
    };

    refreshButtons(ctrl()->anyEdited());

    connect(ctrl(), &OptionsController::editedChanged, this, refreshButtons);
}
