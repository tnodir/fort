#include "connectionspage.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/stat/statisticscontroller.h>
#include <form/stat/statisticswindow.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/connlistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

namespace {

constexpr int CONN_LIST_HEADER_VERSION = 5;

}

ConnectionsPage::ConnectionsPage(StatisticsController *ctrl, QWidget *parent) :
    StatBasePage(ctrl, parent), m_connListModel(new ConnListModel(this))
{
    setupUi();

    updateAutoScroll();
    updateShowHostNames();

    connListModel()->initialize();
}

AppInfoCache *ConnectionsPage::appInfoCache() const
{
    return connListModel()->appInfoCache();
}

void ConnectionsPage::onSaveWindowState(IniUser *ini)
{
    auto header = m_connListView->horizontalHeader();
    ini->setConnListHeader(header->saveState());
    ini->setConnListHeaderVersion(CONN_LIST_HEADER_VERSION);
}

void ConnectionsPage::onRestoreWindowState(IniUser *ini)
{
    if (ini->connListHeaderVersion() == CONN_LIST_HEADER_VERSION) {
        auto header = m_connListView->horizontalHeader();
        header->restoreState(ini->connListHeader());
    }
}

void ConnectionsPage::onRetranslateUi()
{
    m_btEdit->setText(tr("Edit"));
    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actCopy->setText(tr("Copy"));
    m_actAddProgram->setText(tr("Add Program"));
    m_actRemoveConn->setText(tr("Remove"));
    m_actClearAll->setText(tr("Clear All"));

    m_btClearAll->setText(tr("Clear All"));

    m_btOptions->setText(tr("Options"));
    m_cbAutoScroll->setText(tr("Auto scroll"));
    m_cbShowHostNames->setText(tr("Show host names"));

    connListModel()->refresh();

    m_appInfoRow->retranslateUi();
}

void ConnectionsPage::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableConnList();
    setupTableConnListHeader();

    setupHeaderConnections();

    // App Info Row
    setupAppInfoRow();

    // Actions on conns table's current changed
    setupTableConnsChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_connListView, 1);
    layout->addWidget(m_appInfoRow);

    this->setLayout(layout);
}

QLayout *ConnectionsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto menu = ControlUtil::createMenu(this);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actCopy = menu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actAddProgram = menu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    m_actRemoveConn = menu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveConn->setShortcut(Qt::Key_Delete);

    m_actClearAll = menu->addAction(IconCache::icon(":/icons/broom.png"), QString());

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(menu);

    // Toolbar buttons
    m_btClearAll = ControlUtil::createFlatToolButton(":/icons/broom.png");

    connect(m_btClearAll, &QAbstractButton::clicked, m_actClearAll, &QAction::trigger);

    // Options
    setupOptions();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btClearAll);
    layout->addStretch();
    layout->addWidget(m_btOptions);

    return layout;
}

void ConnectionsPage::setupHeaderConnections()
{
    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto rows = m_connListView->selectedRows();
        const auto text = connListModel()->rowsAsFilter(rows);

        GuiUtil::setClipboardData(text);
    });
    connect(m_actCopy, &QAction::triggered, m_connListView, &TableView::copySelectedText);
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto appPath = connListCurrentPath();
        if (!appPath.isEmpty()) {
            windowManager()->openProgramEditForm(appPath, windowManager()->statWindow());
        }
    });
    connect(m_actRemoveConn, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { deleteConn(connListCurrentIndex()); },
                tr("Are you sure to remove connections till this row?"));
    });
    connect(m_actClearAll, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { ctrl()->deleteConn(); }, tr("Are you sure to remove all connections?"));
    });
}

void ConnectionsPage::setupOptions()
{
    setupAutoScroll();
    setupShowHostNames();

    // Menu
    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbAutoScroll, m_cbShowHostNames });

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/widgets.png");
    m_btOptions->setMenu(menu);
}

void ConnectionsPage::setupAutoScroll()
{
    m_cbAutoScroll = ControlUtil::createCheckBox(iniUser()->statAutoScroll(), [&](bool checked) {
        if (iniUser()->statAutoScroll() == checked)
            return;

        iniUser()->setStatAutoScroll(checked);
        confManager()->saveIniUser();

        updateAutoScroll();
    });
}

void ConnectionsPage::setupShowHostNames()
{
    m_cbShowHostNames =
            ControlUtil::createCheckBox(iniUser()->statShowHostNames(), [&](bool checked) {
                if (iniUser()->statShowHostNames() == checked)
                    return;

                iniUser()->setStatShowHostNames(checked);
                confManager()->saveIniUser();

                updateShowHostNames();
            });
}

void ConnectionsPage::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_connListView->setModel(connListModel());

    m_connListView->setMenu(m_btEdit->menu());

    connect(m_connListView, &TableView::doubleClicked, m_actAddProgram, &QAction::trigger);
}

void ConnectionsPage::setupTableConnListHeader()
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
    header->resizeSection(int(ConnListColumn::LocalPort), 80);
    header->resizeSection(int(ConnListColumn::RemoteHostName), 140);
    header->resizeSection(int(ConnListColumn::RemoteIp), 100);
    header->resizeSection(int(ConnListColumn::RemotePort), 80);
    header->resizeSection(int(ConnListColumn::Direction), 30);
    header->resizeSection(int(ConnListColumn::Action), 30);
    header->resizeSection(int(ConnListColumn::Reason), 30);
    header->resizeSection(int(ConnListColumn::Time), 120);

    // Hidden columns
    header->setSectionHidden(int(ConnListColumn::LocalIp), /*hide=*/true);
    header->setSectionHidden(int(ConnListColumn::RemoteIp), /*hide=*/true);

    header->setSectionsMovable(true);
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(int(ConnListColumn::Time), Qt::DescendingOrder);

    connect(header, &QHeaderView::sortIndicatorChanged, this,
            &ConnectionsPage::onTableConnSortClicked);

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this,
            &ConnectionsPage::showTableConnHeaderMenu);
}

void ConnectionsPage::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void ConnectionsPage::setupTableConnsChanged()
{
    const auto refreshTableConnsChanged = [&] {
        const int connIndex = connListCurrentIndex();
        const bool connSelected = (connIndex >= 0);
        m_actCopyAsFilter->setEnabled(connSelected);
        m_actCopy->setEnabled(connSelected);
        m_actAddProgram->setEnabled(connSelected);
        m_actRemoveConn->setEnabled(connSelected);
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

void ConnectionsPage::showTableConnHeaderMenu(const QPoint &pos)
{
    auto menu = ControlUtil::createMenu();
    ControlUtil::deleteOnClose(menu);

    auto header = m_connListView->horizontalHeader();

    setupTableConnHeaderMenuColumns(menu, header);

    menu->addSeparator();

    // Stretch last column
    {
        auto a = new QAction(tr("Stretch last column"), menu);
        a->setCheckable(true);
        a->setChecked(header->stretchLastSection());

        connect(a, &QAction::triggered, this, [&](bool checked) {
            m_connListView->horizontalHeader()->setStretchLastSection(checked);
        });

        menu->addAction(a);
    }

    menu->popup(header->mapToGlobal(pos));
}

void ConnectionsPage::setupTableConnHeaderMenuColumns(QMenu *menu, QHeaderView *header)
{
    const auto switchColumnVisible = [&](bool checked) {
        const auto action = qobject_cast<QAction *>(sender());
        const int column = action->data().toInt();

        auto header = m_connListView->horizontalHeader();

        header->setSectionHidden(column, !checked);
    };

    const bool canHide = (header->hiddenSectionCount() < int(ConnListColumn::Count) - 1);

    for (int i = 0; i < int(ConnListColumn::Count); ++i) {
        const auto name = ConnListModel::columnName(ConnListColumn(i));

        auto a = new QAction(name, menu);
        a->setData(i);

        const bool isHidden = header->isSectionHidden(i);
        a->setCheckable(true);
        a->setChecked(!isHidden);
        a->setEnabled(isHidden || canHide);

        connect(a, &QAction::triggered, this, switchColumnVisible);

        menu->addAction(a);
    }
}

void ConnectionsPage::onTableConnSortClicked(int section, Qt::SortOrder order)
{
    if (section != int(ConnListColumn::Time)) {
        order = connListModel()->sortOrder();
    }

    auto header = m_connListView->horizontalHeader();
    header->setSortIndicator(int(ConnListColumn::Time), order);

    connListModel()->sort(int(ConnListColumn::Time), order);
}

void ConnectionsPage::doAutoScroll()
{
    if (connListModel()->isAscendingOrder()) {
        m_connListView->scrollToBottom();
    } else {
        m_connListView->scrollToTop();
    }
}

void ConnectionsPage::updateAutoScroll()
{
    if (iniUser()->statAutoScroll()) {
        connect(connListModel(), &QAbstractItemModel::rowsInserted, this,
                &ConnectionsPage::doAutoScroll);
        connect(connListModel(), &QAbstractItemModel::modelReset, this,
                &ConnectionsPage::doAutoScroll);

        doAutoScroll();
    } else {
        connListModel()->disconnect(this);
    }
}

void ConnectionsPage::updateShowHostNames()
{
    connListModel()->setResolveAddress(iniUser()->statShowHostNames());
    connListModel()->refresh();
}

void ConnectionsPage::deleteConn(int row)
{
    const auto &connRow = connListModel()->connRowAt(row);
    if (connRow.isNull())
        return;

    ctrl()->deleteConn(connRow.connId);
}

int ConnectionsPage::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

QString ConnectionsPage::connListCurrentPath() const
{
    const auto &connRow = connListModel()->connRowAt(connListCurrentIndex());
    return connRow.isNull() ? QString() : connRow.appPath;
}
