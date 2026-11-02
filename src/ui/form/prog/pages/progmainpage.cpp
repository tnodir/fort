#include "progmainpage.h"

#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/toolbutton.h>
#include <form/prog/programeditcontroller.h>
#include <model/appconnsearchmodel.h>

#include "proggeneralpage.h"
#include "progmorepage.h"
#include "prognetworkpage.h"

namespace {

inline constexpr int generalTabIndex = 0;
inline constexpr int networkTabIndex = 1;
inline constexpr int moreTabIndex = 2;

}

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

void ProgMainPage::setPendingConnsWindowChecked(bool checked)
{
    m_btPendingConns->setChecked(checked);
}

void ProgMainPage::setPendingConnsCount(int count)
{
    m_pendingConnsCount = count;

    m_btPendingConns->setText(QString::number(count));

    updatePendingConnsVisible();
}

void ProgMainPage::setConnsWindowChecked(bool checked)
{
    m_btConnections->setChecked(checked);
}

void ProgMainPage::addConnFilter(const Conn &conn)
{
    if (isNetworkTabEnabled()) {
        selectTab(networkTabIndex);
    }

    m_networkPage->openConnFilterForm(conn);
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
    for (const auto page : std::as_const(m_pages)) {
        page->fillApp(app);
    }
}

void ProgMainPage::onPageInitialize(const App &app)
{
    setNetworkTabEnabled(!app.blocked);

    const bool canFilterApp = AppConnSearchModel::canFilterApp(app);

    m_btPendingConns->setEnabled(canFilterApp);
    m_btConnections->setEnabled(canFilterApp);

    selectTab(generalTabIndex);
}

void ProgMainPage::onRetranslateUi()
{
    m_tabWidget->setTabText(generalTabIndex, tr("General"));
    m_tabWidget->setTabText(networkTabIndex, tr("Network Filters"));
    m_tabWidget->setTabText(moreTabIndex, tr("More"));

    m_btPendingConns->setToolTip(tr("Paused Connections"));
    m_btConnections->setToolTip(tr("Connections"));

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
    m_networkPage = new ProgNetworkPage(ctrl());
    auto morePage = new ProgMorePage(ctrl());

    m_pages = { generalPage, m_networkPage, morePage };

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(generalPage, QString());
    m_tabWidget->addTab(m_networkPage, QString());
    m_tabWidget->addTab(morePage, QString());

    // Paused Connections button: with their count
    m_btPendingConns = ControlUtil::createIconToolButton(":/icons/pause.png");
    m_btPendingConns->setCheckable(true);
    m_btPendingConns->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    setPendingConnsCount(0);

    connect(m_btPendingConns, &QAbstractButton::clicked, this,
            &ProgMainPage::pendingConnsWindowToggled);
    connect(m_btPendingConns, &QAbstractButton::toggled, this,
            &ProgMainPage::updatePendingConnsVisible);

    // Connections button
    m_btConnections = ControlUtil::createCheckableIconToolButton(
            ":/icons/disconnect.png", ":/icons/connect.png");

    connect(m_btConnections, &QAbstractButton::clicked, this, &ProgMainPage::connsWindowToggled);

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto cornerLayout = ControlUtil::createHLayoutByWidgets(
            { m_btPendingConns, m_btConnections, ControlUtil::createVSeparator(), m_btMenu },
            /*margin=*/0);

    auto cornerWidget = new QWidget();
    cornerWidget->setLayout(cornerLayout);

    m_tabWidget->setCornerWidget(cornerWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this,
            [&](int tabIndex) { pageAt(tabIndex)->onPageActivated(); });
}

QLayout *ProgMainPage::setupButtonsLayout()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] { ctrl()->saveChanges(); });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = ControlUtil::createButton(QString(), [&] { ctrl()->closeWindow(); });

    auto layout = ControlUtil::createHLayoutByWidgets({ /*stretch*/ nullptr, m_btOk, m_btCancel });

    return layout;
}

bool ProgMainPage::isNetworkTabEnabled() const
{
    // Enabled unless the action is Block or Kill Process
    return m_tabWidget->isTabEnabled(networkTabIndex);
}

void ProgMainPage::setNetworkTabEnabled(bool enabled)
{
    m_tabWidget->setTabEnabled(networkTabIndex, enabled);
}

void ProgMainPage::updatePendingConnsVisible()
{
    // Shown with the pending connections or their open window
    const bool visible = (m_pendingConnsCount > 0 || m_btPendingConns->isChecked());

    m_btPendingConns->setVisible(visible);
}

ProgBasePage *ProgMainPage::pageAt(int index) const
{
    return m_pages.at(index);
}
