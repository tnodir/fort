#include "progmainpage.h"

#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/firewallconf.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/controls/toolbutton.h>
#include <form/prog/programscontroller.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/appconnlistmodel.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/osutil.h>

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

    m_btSwitchWildcard->setToolTip(tr("Switch Wildcard"));
    m_btConnections->setToolTip(tr("Connections"));
    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));
}

void ProgMainPage::retranslateTableConnListMenu()
{
    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actCopy->setText(tr("Copy"));
    m_actLookupIp->setText(tr("Lookup IP"));

    m_appInfoRow->retranslateUi();
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

    m_pages = { generalPage, networkPage, morePage };

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(generalPage, QString());
    m_tabWidget->addTab(networkPage, QString());
    m_tabWidget->addTab(morePage, QString());

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

    // Connections
    setupConnections();

    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] { ctrl()->saveChanges(); });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = ControlUtil::createButton(QString(), [&] { ctrl()->closeWindow(); });

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btSwitchWildcard, m_btConnections,
            /*stretch*/ nullptr, m_btOk, m_btCancel });

    return layout;
}

void ProgMainPage::setupSwitchWildcard()
{
    m_btSwitchWildcard = ControlUtil::createIconToolButton(
            ":/icons/coding.png", [&] { ctrl()->switchWildcard(); });

    m_btSwitchWildcard->setCheckable(true);
}

void ProgMainPage::setupConnections()
{
    m_connectionsLayout = new QVBoxLayout();

    auto menu = ControlUtil::createMenuByLayout(m_connectionsLayout, this);

    m_btConnections = ControlUtil::createButton(":/icons/connect.png");
    m_btConnections->setShortcut(QKeyCombination(Qt::CTRL | Qt::ALT, Qt::Key_C));
    m_btConnections->setMenu(menu);

    connect(menu, &QMenu::aboutToShow, this, &ProgMainPage::setupConnectionsMenuLayout);
    connect(menu, &QMenu::aboutToHide, this, &ProgMainPage::closeConnectionsMenuLayout);
}

void ProgMainPage::setupConnectionsMenuLayout()
{
    Q_ASSERT(m_connectionsLayout->isEmpty());

    setupConnectionsModel();
    setupTableConnList();
    setupTableConnListHeader();
    setupConnectionsAppInfoRow();

    setupTableConnsChanged();

    retranslateTableConnListMenu();

    m_connListView->setFixedWidth(800);

    m_connectionsLayout->addWidget(m_connListView, 1);
    m_connectionsLayout->addWidget(m_appInfoRow);
}

void ProgMainPage::closeConnectionsMenuLayout()
{
    ControlUtil::clearLayout(m_connectionsLayout);

    m_appConnListModel->deleteLater();
}

void ProgMainPage::setupConnectionsModel()
{
    m_appConnListModel = new AppConnListModel(this);

    appConnListModel()->setConfAppId(app().appId);
    if (!app().isWildcard) {
        appConnListModel()->setAppPath(app().appPath);
    }
    appConnListModel()->setResolveAddress(true);
    appConnListModel()->initialize();
}

void ProgMainPage::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_connListView->setModel(appConnListModel());

    setupTableConnListMenu();
}

void ProgMainPage::setupTableConnListMenu()
{
    auto menu = ControlUtil::createMenu(m_connListView);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actCopy = menu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actLookupIp = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actLookupIp->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_L);

    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto rows = m_connListView->selectedRows();
        const auto text = appConnListModel()->rowsAsFilter(rows);

        GuiUtil::setClipboardData(text);
    });
    connect(m_actCopy, &QAction::triggered, m_connListView, &TableView::copySelectedText);
    connect(m_actLookupIp, &QAction::triggered, this, [&] {
        const auto row = m_connListView->currentRow();
        const auto index = appConnListModel()->index(row, int(ConnListColumn::RemoteIp));
        const auto textIp = appConnListModel()->data(index).toString();

        OsUtil::openIpLocationUrl(textIp);
    });

    m_connListView->setMenu(menu);

    m_connListView->addAction(m_actCopyAsFilter);
    m_connListView->addAction(m_actLookupIp);
}

void ProgMainPage::setupTableConnListHeader()
{
    auto header = m_connListView->horizontalHeader();

    header->setSectionResizeMode(int(ConnListColumn::Program), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::ProcessId), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Protocol), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalPort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemotePort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Direction), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Action), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Reason), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Time), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(ConnListColumn::Program), 300);
    header->resizeSection(int(ConnListColumn::ProcessId), 60);
    header->resizeSection(int(ConnListColumn::Protocol), 60);
    header->resizeSection(int(ConnListColumn::LocalHostName), 140);
    header->resizeSection(int(ConnListColumn::LocalIp), 100);
    header->resizeSection(int(ConnListColumn::LocalPort), 60);
    header->resizeSection(int(ConnListColumn::RemoteHostName), 150);
    header->resizeSection(int(ConnListColumn::RemoteIp), 100);
    header->resizeSection(int(ConnListColumn::RemotePort), 70);
    header->resizeSection(int(ConnListColumn::Direction), 30);
    header->resizeSection(int(ConnListColumn::Action), 30);
    header->resizeSection(int(ConnListColumn::Reason), 30);
    header->resizeSection(int(ConnListColumn::Time), 80);

    // Hidden columns
    header->setSectionHidden(int(ConnListColumn::Program), /*hide=*/true);
    header->setSectionHidden(int(ConnListColumn::LocalHostName), /*hide=*/true);
}

void ProgMainPage::setupConnectionsAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, m_appInfoRow, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, m_appInfoRow, refreshAppInfoVersion);
}

void ProgMainPage::setupTableConnsChanged()
{
    const auto refreshTableConnsChanged = [&] {
        const int connIndex = m_connListView->currentRow();
        const bool connSelected = (connIndex >= 0);
        m_actCopyAsFilter->setEnabled(connSelected);
        m_actCopy->setEnabled(connSelected);
        m_actLookupIp->setEnabled(connSelected);
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

int ProgMainPage::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

const ConnRow &ProgMainPage::connListCurrentRow() const
{
    return m_appConnListModel->connRowAt(connListCurrentIndex());
}

QString ProgMainPage::connListCurrentPath() const
{
    const auto &connRow = connListCurrentRow();
    return connRow.isNull() ? QString() : connRow.appPath;
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
