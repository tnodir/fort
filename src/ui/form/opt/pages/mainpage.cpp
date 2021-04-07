#include "mainpage.h"

#include <QIcon>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "../../../fortsettings.h"
#include "../../../util/iconcache.h"
#include "../../controls/controlutil.h"
#include "../optionscontroller.h"
#include "addressespage.h"
#include "applicationspage.h"
#include "optionspage.h"
#include "rulespage.h"
#include "schedulepage.h"
#include "statisticspage.h"

MainPage::MainPage(OptionsController *ctrl, QWidget *parent) : BasePage(ctrl, parent)
{
    setupUi();
}

void MainPage::onRetranslateUi()
{
    m_tabBar->setTabText(0, tr("Options"));
    m_tabBar->setTabText(1, tr("IPv4 Addresses"));
    m_tabBar->setTabText(2, tr("Network Rules"));
    m_tabBar->setTabText(3, tr("Application Groups"));
    m_tabBar->setTabText(4, tr("Statistics"));
    m_tabBar->setTabText(5, tr("Schedule"));

    m_btLogs->setText(tr("Logs"));
    m_btProfile->setText(tr("Profile"));
    m_btStat->setText(tr("Statistics"));
    m_btReleases->setText(tr("Releases"));

    m_btOk->setText(tr("OK"));
    m_btApply->setText(tr("Apply"));
    m_btCancel->setText(tr("Cancel"));
}

void MainPage::setupUi()
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

void MainPage::setupTabBar()
{
    auto optionsPage = ControlUtil::wrapToScrollArea(new OptionsPage(ctrl()));
    auto addressesPage = new AddressesPage(ctrl());
    auto rulesPage = new RulesPage(ctrl());
    auto applicationsPage = new ApplicationsPage(ctrl());
    auto statisticsPage = new StatisticsPage(ctrl());
    auto schedulePage = new SchedulePage(ctrl());

    m_tabBar = new QTabWidget();
    m_tabBar->addTab(optionsPage, IconCache::icon(":/icons/cog.png"), QString());
    m_tabBar->addTab(addressesPage, IconCache::icon(":/icons/map-marker.png"), QString());
    m_tabBar->addTab(rulesPage, IconCache::icon(":/icons/task-list.png"), QString());
    m_tabBar->addTab(applicationsPage, IconCache::icon(":/icons/window.png"), QString());
    m_tabBar->addTab(statisticsPage, IconCache::icon(":/icons/database.png"), QString());
    m_tabBar->addTab(schedulePage, IconCache::icon(":/icons/clock.png"), QString());

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_tabBar->setTabVisible(2, false); // TODO: Impl. Network Rules
#endif
}

QLayout *MainPage::setupDialogButtons()
{
    auto buttonsLayout = new QHBoxLayout();

    m_btLogs = ControlUtil::createLinkButton(":/icons/folder-open.png", settings()->logsPath());
    m_btProfile =
            ControlUtil::createLinkButton(":/icons/folder-open.png", settings()->profilePath());
    m_btStat = ControlUtil::createLinkButton(":/icons/folder-open.png", settings()->statPath());
    m_btReleases = ControlUtil::createLinkButton(":/icons/github.png", settings()->appUpdatesUrl());

    connect(m_btLogs, &QAbstractButton::clicked, this, &MainPage::onLinkClicked);
    connect(m_btProfile, &QAbstractButton::clicked, this, &MainPage::onLinkClicked);
    connect(m_btStat, &QAbstractButton::clicked, this, &MainPage::onLinkClicked);
    connect(m_btReleases, &QAbstractButton::clicked, this, &MainPage::onLinkClicked);

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

    setupOkApplyButtons();

    buttonsLayout->addWidget(m_btOk);
    buttonsLayout->addWidget(m_btApply);
    buttonsLayout->addWidget(m_btCancel);

    return buttonsLayout;
}

void MainPage::setupOkApplyButtons()
{
    const auto refreshOkApplyButtons = [&](bool anyEdited) {
        m_btOk->setEnabled(anyEdited);
        m_btApply->setEnabled(anyEdited);
    };

    refreshOkApplyButtons(false);

    connect(ctrl(), &OptionsController::editedChanged, this, refreshOkApplyButtons);
}
