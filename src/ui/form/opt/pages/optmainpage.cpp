#include "optmainpage.h"

#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "addressespage.h"
#include "applicationspage.h"
#include "graphpage.h"
#include "ifacepage.h"
#include "optionspage.h"
#include "schedulepage.h"
#include "statisticspage.h"

OptMainPage::OptMainPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void OptMainPage::selectTab(int index)
{
    m_tabWidget->setCurrentIndex(index);
}

void OptMainPage::onRetranslateUi()
{
    m_tabWidget->setTabText(0, tr("Options"));
    m_tabWidget->setTabText(1, tr("Interface"));
    m_tabWidget->setTabText(2, tr("IP Addresses"));
    m_tabWidget->setTabText(3, tr("Application Groups"));
    m_tabWidget->setTabText(4, tr("Statistics"));
    m_tabWidget->setTabText(5, tr("Traffic Graph"));
    m_tabWidget->setTabText(6, tr("Schedule"));

    m_btBackup->setText(tr("Backup"));
    m_actExport->setText(tr("Export"));
    m_actImport->setText(tr("Import"));
    m_actImportApps->setText(tr("Import new programs"));

    m_btDefault->setText(tr("Default"));
    m_actDefaultAll->setText(tr("Reset to default all options"));
    m_actDefaultTab->setText(tr("Reset to default current tab"));

    m_btOk->setText(tr("OK"));
    m_btApply->setText(tr("Apply"));
    m_btCancel->setText(tr("Cancel"));
}

void OptMainPage::setupUi()
{
    // Main Tab Bar
    setupTabBar();

    // Dialog buttons
    auto buttonsLayout = setupButtonsLayout();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addWidget(m_tabWidget);
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);
}

void OptMainPage::setupTabBar()
{
    auto optionsPage = new OptionsPage(ctrl());
    auto ifacePage = new IfacePage(ctrl());
    auto addressesPage = new AddressesPage(ctrl());
    auto applicationsPage = new ApplicationsPage(ctrl());
    auto statisticsPage = new StatisticsPage(ctrl());
    auto graphPage = new GraphPage(ctrl());
    auto schedulePage = new SchedulePage(ctrl());

    m_pages = {
        optionsPage,
        ifacePage,
        addressesPage,
        applicationsPage,
        statisticsPage,
        graphPage,
        schedulePage,
    };

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(ControlUtil::wrapToScrollArea(optionsPage),
            IconCache::icon(":/icons/cog.png"), QString());
    m_tabWidget->addTab(ControlUtil::wrapToScrollArea(ifacePage),
            IconCache::icon(":/icons/interface_preferences.png"), QString());
    m_tabWidget->addTab(addressesPage, IconCache::icon(":/icons/ip.png"), QString());
    m_tabWidget->addTab(
            applicationsPage, IconCache::icon(":/icons/application_double.png"), QString());
    m_tabWidget->addTab(ControlUtil::wrapToScrollArea(statisticsPage),
            IconCache::icon(":/icons/chart_bar.png"), QString());
    m_tabWidget->addTab(graphPage, IconCache::icon(":/icons/action_log.png"), QString());
    m_tabWidget->addTab(schedulePage, IconCache::icon(":/icons/clock.png"), QString());

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    m_tabWidget->setCornerWidget(m_btMenu);

    connect(m_tabWidget, &QTabWidget::currentChanged, this,
            [&](int tabIndex) { pageAt(tabIndex)->onPageActivated(); });
}

QLayout *OptMainPage::setupButtonsLayout()
{
    setupBackup();
    setupDefault();

    m_btOk = new QPushButton();
    m_btApply = new QPushButton();
    m_btCancel = new QPushButton();

    connect(m_btOk, &QAbstractButton::clicked, ctrl(), &OptionsController::saveChanges);
    connect(m_btApply, &QAbstractButton::clicked, ctrl(), &OptionsController::applyChanges);
    connect(m_btCancel, &QAbstractButton::clicked, ctrl(), &OptionsController::closeWindow);

    setupApplyCancelButtons();

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btBackup);
    layout->addWidget(m_btDefault);
    layout->addStretch();
    layout->addWidget(m_btOk);
    layout->addWidget(m_btApply);
    layout->addWidget(m_btCancel);

    return layout;
}

void OptMainPage::setupBackup()
{
    auto menu = ControlUtil::createMenu(this);

    m_actExport = menu->addAction(IconCache::icon(":/icons/disk.png"), QString());
    m_actImport = menu->addAction(IconCache::icon(":/icons/folder.png"), QString());
    m_actImportApps = menu->addAction(IconCache::icon(":/icons/application.png"), QString());

    connect(m_actExport, &QAction::triggered, ctrl(), &OptionsController::exportBackup);
    connect(m_actImport, &QAction::triggered, ctrl(), &OptionsController::confirmImportBackup);
    connect(m_actImportApps, &QAction::triggered, ctrl(),
            &OptionsController::confirmImportAppsBackup);

    m_btBackup = new QPushButton();
    m_btBackup->setMenu(menu);
}

void OptMainPage::setupDefault()
{
    auto menu = ControlUtil::createMenu(this);

    m_actDefaultAll = menu->addAction(QString());
    m_actDefaultTab = menu->addAction(QString());

    connect(m_actDefaultAll, &QAction::triggered, this, [&] { emit ctrl()->resetToDefault(); });
    connect(m_actDefaultTab, &QAction::triggered, this, [&] { currentPage()->onResetToDefault(); });

    m_btDefault = new QPushButton();
    m_btDefault->setMenu(menu);
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

OptBasePage *OptMainPage::currentPage() const
{
    return pageAt(m_tabWidget->currentIndex());
}

OptBasePage *OptMainPage::pageAt(int index) const
{
    return m_pages.at(index);
}
