#include "connectionswindow.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/confmanager.h>
#include <conf/filtersimconn.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/filtersim/filtersimwindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/connsearchmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/osutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "connectionscontroller.h"

using namespace Fort;

namespace {

inline constexpr int CONN_LIST_HEADER_VERSION = 5;

void showFilterSimWindow(const ConnRow &connRow)
{
    auto windowManager = Fort::windowManager();

    if (!windowManager->showFilterSimWindow())
        return;

    FilterSimConn simConn;
    simConn.appPath = connRow.appPath;

    FORT_CONF_META_CONN &conn = simConn.conn;
    conn.inbound = connRow.inbound;
    conn.isIPv6 = connRow.isIPv6;
    conn.is_loopback = connRow.loopback;
    conn.ip_proto = connRow.ipProto;
    conn.local_port = connRow.localPort;
    conn.remote_port = connRow.remotePort;
    conn.local_ip = connRow.localIp;
    conn.remote_ip = connRow.remoteIp;

    windowManager->filterSimWindow()->initialize(simConn);
}

}

ConnectionsWindow::ConnectionsWindow(QWidget *parent) :
    FormWindow(parent),
    m_ctrl(new ConnectionsController(this)),
    m_connListModel(new ConnSearchModel(this))
{
    setupUi();

    updateAutoScroll();
    updateShowHostNames();

    connListModel()->initialize();

    setupController();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());
}

void ConnectionsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setConnWindowGeometry(stateWatcher()->geometry());
    iniUser.setConnWindowMaximized(stateWatcher()->maximized());

    auto header = m_connListView->horizontalHeader();
    iniUser.setConnListHeader(header->saveState());
    iniUser.setConnListHeaderVersion(CONN_LIST_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ConnectionsWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(1024, 768), iniUser.connWindowGeometry(), iniUser.connWindowMaximized());

    if (iniUser.connListHeaderVersion() == CONN_LIST_HEADER_VERSION) {
        auto header = m_connListView->horizontalHeader();
        header->restoreState(iniUser.connListHeader());
    }
}

void ConnectionsWindow::setupController()
{
    connect(ctrl(), &ConnectionsController::retranslateUi, this, &ConnectionsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void ConnectionsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actCopy->setText(tr("Copy"));
    m_actLookupIp->setText(tr("Lookup IP"));

    m_actAddProgram->setText(tr("Add Program"));
    m_actFilterSim->setText(tr("Filter Simulator"));
    m_actRemoveConn->setText(tr("Remove"));
    m_actClearAll->setText(tr("Clear All"));
    m_actFind->setText(tr("Find"));

    m_btClearAll->setText(tr("Clear All"));

    m_editSearch->setPlaceholderText(tr("Search") + " /");

    m_btListOptions->setText(tr("Options"));
    m_cbAutoScroll->setText(tr("Auto scroll"));
    m_cbShowHostNames->setText(tr("Show host names"));

    connListModel()->refresh();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Connections"));
}

void ConnectionsWindow::setupUi()
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

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ConnectionsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto menu = ControlUtil::createMenu(this);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actCopy = menu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actLookupIp = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actLookupIp->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_L);

    m_actAddProgram = menu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actFilterSim = menu->addAction(IconCache::icon(":/icons/filter.png"), QString());

    m_actRemoveConn = menu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveConn->setShortcut(Qt::Key_Delete);

    m_actClearAll = menu->addAction(IconCache::icon(":/icons/broom.png"), QString());

    menu->addSeparator();

    m_actFind = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actFind->setShortcut(QKeySequence::Find);

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(menu);

    // Toolbar buttons
    m_btClearAll = ControlUtil::createFlatToolButton(":/icons/broom.png");

    connect(m_btClearAll, &QAbstractButton::clicked, m_actClearAll, &QAction::trigger);

    // Search
    setupEditSearch();

    // List Options
    setupListOptions();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton(3);

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btClearAll);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_editSearch);
    layout->addStretch();
    layout->addWidget(m_btListOptions);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btOptions);
    layout->addWidget(m_btMenu);

    return layout;
}

void ConnectionsWindow::setupHeaderConnections()
{
    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto rows = m_connListView->selectedRows();
        const auto text = connListModel()->rowsAsFilter(rows);

        GuiUtil::setClipboardData(text);
    });
    connect(m_actCopy, &QAction::triggered, m_connListView, &TableView::copySelectedText);
    connect(m_actLookupIp, &QAction::triggered, this, [&] {
        const auto row = m_connListView->currentRow();
        const auto index = connListModel()->index(row, int(ConnListColumn::RemoteIp));
        const auto textIp = connListModel()->data(index).toString();

        OsUtil::openIpLocationUrl(textIp);
    });

    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto &connRow = currentConnRow();

        if (!connRow.isNull()) {
            windowManager()->openProgramEditForm(connRow.appPath, connRow.confAppId, this);
        }
    });
    connect(m_actFilterSim, &QAction::triggered, this, [&] {
        const auto &connRow = currentConnRow();

        if (!connRow.isNull()) {
            showFilterSimWindow(connRow);
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
    connect(m_actFind, &QAction::triggered, this, [&] {
        m_editSearch->setFocus();
        m_editSearch->selectAll();
    });
}

void ConnectionsWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { connListModel()->setTextFilter(text); });
    m_editSearch->setClearButtonEnabled(true);
    m_editSearch->setMaxLength(200);
    m_editSearch->setMinimumWidth(100);
    m_editSearch->setMaximumWidth(200);
}

void ConnectionsWindow::setupListOptions()
{
    setupAutoScroll();
    setupShowHostNames();

    // Menu
    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbAutoScroll, m_cbShowHostNames });

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btListOptions = ControlUtil::createButton(":/icons/widgets.png");
    m_btListOptions->setMenu(menu);
}

void ConnectionsWindow::setupAutoScroll()
{
    m_cbAutoScroll = ControlUtil::createCheckBox(iniUser().connAutoScroll(), [&](bool checked) {
        if (iniUser().connAutoScroll() == checked)
            return;

        iniUser().setConnAutoScroll(checked);
        confManager()->saveIniUser();

        updateAutoScroll();
    });
}

void ConnectionsWindow::setupShowHostNames()
{
    m_cbShowHostNames =
            ControlUtil::createCheckBox(iniUser().connShowHostNames(), [&](bool checked) {
                if (iniUser().connShowHostNames() == checked)
                    return;

                iniUser().setConnShowHostNames(checked);
                confManager()->saveIniUser();

                updateShowHostNames();
            });
}

void ConnectionsWindow::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_connListView->setModel(connListModel());

    m_connListView->setMenu(m_btEdit->menu());

    connect(m_connListView, &TableView::doubleClicked, m_actAddProgram, &QAction::trigger);
}

void ConnectionsWindow::setupTableConnListHeader()
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
            &ConnectionsWindow::onTableConnSortClicked);

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this,
            &ConnectionsWindow::showTableConnHeaderMenu);
}

void ConnectionsWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        const auto &connRow = currentConnRow();
        const auto appPath = connRow.isNull() ? QString() : connRow.appPath;

        m_appInfoRow->refreshAppInfoVersion(appPath, appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void ConnectionsWindow::setupTableConnsChanged()
{
    const auto refreshTableConnsChanged = [&] {
        const int connIndex = connListCurrentIndex();
        const bool connSelected = (connIndex >= 0);

        m_actCopyAsFilter->setEnabled(connSelected);
        m_actCopy->setEnabled(connSelected);
        m_actLookupIp->setEnabled(connSelected);
        m_actAddProgram->setEnabled(connSelected);
        m_actFilterSim->setEnabled(connSelected);

        /* Removing till the row would remove the filtered out connections too */
        m_actRemoveConn->setEnabled(connSelected && !connListModel()->isFiltering());
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

void ConnectionsWindow::showTableConnHeaderMenu(const QPoint &pos)
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

void ConnectionsWindow::setupTableConnHeaderMenuColumns(QMenu *menu, QHeaderView *header)
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

void ConnectionsWindow::onTableConnSortClicked(int section, Qt::SortOrder order)
{
    if (section != int(ConnListColumn::Time)) {
        order = connListModel()->sortOrder();
    }

    auto header = m_connListView->horizontalHeader();
    header->setSortIndicator(int(ConnListColumn::Time), order);

    connListModel()->sort(int(ConnListColumn::Time), order);
}

void ConnectionsWindow::doAutoScroll()
{
    if (connListModel()->isAscendingOrder()) {
        m_connListView->scrollToBottom();
    } else {
        m_connListView->scrollToTop();
    }
}

void ConnectionsWindow::saveTopRow()
{
    m_topRowIndex = m_connListView->indexAt(QPoint(0, 0));

    if (!m_topRowIndex.isValid())
        return;

    m_topRowOffset = m_connListView->scrollOffset(m_topRowIndex.row());
}

void ConnectionsWindow::restoreTopRow()
{
    if (!m_topRowIndex.isValid())
        return;

    m_connListView->setScrollOffset(m_topRowIndex.row(), m_topRowOffset);
}

void ConnectionsWindow::updateAutoScroll()
{
    connListModel()->disconnect(this);

    if (iniUser().connAutoScroll()) {
        setupAutoScrollConnections();
    } else {
        setupKeepScrollConnections();
    }
}

void ConnectionsWindow::setupAutoScrollConnections()
{
    connect(connListModel(), &QAbstractItemModel::rowsInserted, this,
            &ConnectionsWindow::doAutoScroll);
    connect(connListModel(), &QAbstractItemModel::modelReset, this,
            &ConnectionsWindow::doAutoScroll);

    doAutoScroll();
}

void ConnectionsWindow::setupKeepScrollConnections()
{
    /* Keep the visible rows in place, when rows are inserted or removed above them */
    connect(connListModel(), &QAbstractItemModel::rowsAboutToBeInserted, this,
            &ConnectionsWindow::saveTopRow);
    connect(connListModel(), &QAbstractItemModel::rowsInserted, this,
            &ConnectionsWindow::restoreTopRow);
    connect(connListModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this,
            &ConnectionsWindow::saveTopRow);
    connect(connListModel(), &QAbstractItemModel::rowsRemoved, this,
            &ConnectionsWindow::restoreTopRow);
}

void ConnectionsWindow::updateShowHostNames()
{
    connListModel()->setResolveAddress(iniUser().connShowHostNames());
    connListModel()->refresh();
}

void ConnectionsWindow::deleteConn(int row)
{
    const auto &connRow = connListModel()->connRowAt(row);
    if (connRow.isNull())
        return;

    ctrl()->deleteConn(connRow.connId);
}

int ConnectionsWindow::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

const ConnRow &ConnectionsWindow::currentConnRow() const
{
    return connListModel()->connRowAt(connListCurrentIndex());
}
