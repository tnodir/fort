#include "progmainpage.h"

#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/toolbutton.h>
#include <form/prog/programscontroller.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "progconnlistpage.h"
#include "proggeneralpage.h"
#include "progmorepage.h"
#include "prognetworkpage.h"

ProgMainPage::ProgMainPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent)
{
    setupUi();
    setupController();
}

void ProgMainPage::selectTab(int index)
{
    m_tabWidget->setCurrentIndex(index);
}

void ProgMainPage::onValidateFields(bool &ok)
{
    for (int i = 0, n = m_tabWidget->count(); i < n; ++i) {
        const auto page = pageAt(i);
        if (!page->validateFields()) {
            selectTab(i);
            return;
        }
    }

    ok = true;
}

void ProgMainPage::onFillApp(App &app)
{
    for (int i = 0, n = m_tabWidget->count(); i < n; ++i) {
        const auto page = pageAt(i);
        page->fillApp(app);
    }
}

void ProgMainPage::onPageInitialize(const App &app)
{
    m_btSwitchWildcard->setChecked(isWildcard());
    m_btSwitchWildcard->setEnabled(isSingleSelection());

    setNetworkTabEnabled(!app.blocked);

    selectTab(0);
}

void ProgMainPage::onRetranslateUi()
{
    m_tabWidget->setTabText(0, tr("General"));
    m_tabWidget->setTabText(1, tr("Network Filters"));
    m_tabWidget->setTabText(2, tr("More"));
    m_tabWidget->setTabText(3, tr("Connections"));

    m_btSwitchWildcard->setToolTip(tr("Switch Wildcard"));
    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));
}

void ProgMainPage::setupController()
{
    connect(ctrl(), &ProgramEditController::validateFields, this, &ProgMainPage::onValidateFields);
    connect(ctrl(), &ProgramEditController::fillApp, this, &ProgMainPage::onFillApp);

    connect(ctrl(), &ProgramEditController::allowToggled, this,
            &ProgMainPage::setNetworkTabEnabled);
}

void ProgMainPage::setupUi()
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

void ProgMainPage::setupTabBar()
{
    auto generalPage = new ProgGeneralPage(ctrl());
    auto networkPage = new ProgNetworkPage(ctrl());
    auto morePage = new ProgMorePage(ctrl());
    auto connsPage = new ProgConnListPage(ctrl());

    m_pages = { generalPage, networkPage, morePage, connsPage };

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(generalPage, QString());
    m_tabWidget->addTab(networkPage, QString());
    m_tabWidget->addTab(morePage, QString());
    m_tabWidget->addTab(connsPage, IconCache::icon(":/icons/connect.png"), QString());

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    m_tabWidget->setCornerWidget(m_btMenu);

    connect(m_tabWidget, &QTabWidget::currentChanged, this,
            [&](int tabIndex) { pageAt(tabIndex)->onPageActivated(); });
}

QLayout *ProgMainPage::setupButtonsLayout()
{
    // Switch Wildcard
    setupSwitchWildcard();

    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] { ctrl()->saveChanges(); });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = ControlUtil::createButton(QString(), [&] { ctrl()->closeWindow(); });

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btSwitchWildcard,
            /*stretch*/ nullptr, m_btOk, m_btCancel });

    return layout;
}

void ProgMainPage::setupSwitchWildcard()
{
    m_btSwitchWildcard = ControlUtil::createIconToolButton(
            ":/icons/coding.png", [&] { ctrl()->switchWildcard(); });

    m_btSwitchWildcard->setCheckable(true);
}

void ProgMainPage::setNetworkTabEnabled(bool enabled)
{
    m_tabWidget->setTabEnabled(1, enabled);
}

ProgBasePage *ProgMainPage::currentPage() const
{
    return pageAt(m_tabWidget->currentIndex());
}

ProgBasePage *ProgMainPage::pageAt(int index) const
{
    return m_pages.at(index);
}
