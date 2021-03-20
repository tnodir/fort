#include "connectionswindow.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/connlistmodel.h"
#include "../../util/app/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../controls/appinforow.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "connectionscontroller.h"

namespace {

#define CONN_LIST_HEADER_VERSION 1

}

ConnectionsWindow::ConnectionsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent), m_ctrl(new ConnectionsController(fortManager, this))
{
    setupUi();
    setupController();

    syncAutoScroll();
    syncShowHostNames();
}

void ConnectionsWindow::setupController()
{
    connect(ctrl(), &ConnectionsController::retranslateUi, this,
            &ConnectionsWindow::onRetranslateUi);

    connect(this, &ConnectionsWindow::aboutToClose, fortManager(),
            &FortManager::closeConnectionsWindow);

    connect(fortManager(), &FortManager::afterSaveConnWindowState, this,
            &ConnectionsWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreConnWindowState, this,
            &ConnectionsWindow::onRestoreWindowState);

    emit ctrl()->retranslateUi();
}

void ConnectionsWindow::onSaveWindowState()
{
    auto header = m_connListView->horizontalHeader();
    settings()->setConnListHeader(header->saveState());
    settings()->setConnListHeaderVersion(CONN_LIST_HEADER_VERSION);
}

void ConnectionsWindow::onRestoreWindowState()
{
    if (settings()->connListHeaderVersion() != CONN_LIST_HEADER_VERSION)
        return;

    auto header = m_connListView->horizontalHeader();
    header->restoreState(settings()->connListHeader());
}

void ConnectionsWindow::onRetranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actCopy->setText(tr("Copy"));
    m_actRemoveConn->setText(tr("Remove"));
    m_actClearConns->setText(tr("Clear All"));

    m_btLogOptions->setText(tr("Options"));
    m_cbLogAllowedIp->setText(tr("Collect allowed connections"));
    m_cbLogBlockedIp->setText(tr("Collect blocked connections"));
    m_cbAutoScroll->setText(tr("Auto scroll"));
    m_cbShowHostNames->setText(tr("Show host names"));

    connListModel()->refresh();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Connections"));
}

void ConnectionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableConnList();
    setupTableConnListHeader();
    layout->addWidget(m_connListView, 1);

    // App Info Row
    setupAppInfoRow();
    layout->addWidget(m_appInfoRow);

    // Actions on conns table's current changed
    setupTableConnsChanged();

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png", ":/icons/connect.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ConnectionsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actCopy = editMenu->addAction(IconCache::icon(":/icons/copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actRemoveConn = editMenu->addAction(IconCache::icon(":/icons/sign-delete.png"), QString());
    m_actRemoveConn->setShortcut(Qt::Key_Delete);

    m_actClearConns = editMenu->addAction(IconCache::icon(":/icons/trash.png"), QString());

    connect(m_actCopy, &QAction::triggered, this,
            [&] { GuiUtil::setClipboardData(m_connListView->selectedText()); });
    connect(m_actRemoveConn, &QAction::triggered, this, [&] {
        if (fortManager()->showQuestionBox(
                    tr("Are you sure to remove connections till this row?"))) {
            deleteConn(m_connListView->currentRow());
        }
    });
    connect(m_actClearConns, &QAction::triggered, this, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove all connections?"))) {
            connListModel()->clear();
        }
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Log Options
    setupLogOptions();

    layout->addWidget(m_btEdit);
    layout->addStretch();
    layout->addWidget(m_btLogOptions);

    return layout;
}

void ConnectionsWindow::setupLogOptions()
{
    setupLogAllowedIp();
    setupLogBlockedIp();
    setupAutoScroll();
    setupShowHostNames();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbLogAllowedIp, m_cbLogBlockedIp,
        ControlUtil::createSeparator(), m_cbAutoScroll, m_cbShowHostNames };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btLogOptions = ControlUtil::createButton(":/icons/wrench.png");
    m_btLogOptions->setMenu(menu);
}

void ConnectionsWindow::setupLogAllowedIp()
{
    m_cbLogAllowedIp = ControlUtil::createCheckBox(conf()->logAllowedIp(), [&](bool checked) {
        if (conf()->logAllowedIp() == checked)
            return;

        conf()->setLogAllowedIp(checked);

        fortManager()->applyConfImmediateFlags();
    });

    m_cbLogAllowedIp->setVisible(false); // TODO: Collect allowed connections
}

void ConnectionsWindow::setupLogBlockedIp()
{
    m_cbLogBlockedIp = ControlUtil::createCheckBox(conf()->logBlockedIp(), [&](bool checked) {
        if (conf()->logBlockedIp() == checked)
            return;

        conf()->setLogBlockedIp(checked);

        fortManager()->applyConfImmediateFlags();
    });
}

void ConnectionsWindow::setupAutoScroll()
{
    m_cbAutoScroll = ControlUtil::createCheckBox(settings()->connAutoScroll(), [&](bool checked) {
        if (settings()->connAutoScroll() == checked)
            return;

        settings()->setConnAutoScroll(checked);

        syncAutoScroll();
    });
}

void ConnectionsWindow::setupShowHostNames()
{
    m_cbShowHostNames =
            ControlUtil::createCheckBox(settings()->connShowHostNames(), [&](bool checked) {
                if (settings()->connShowHostNames() == checked)
                    return;

                settings()->setConnShowHostNames(checked);

                syncShowHostNames();
            });
}

void ConnectionsWindow::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setAlternatingRowColors(true);
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    // TODO: Select the allowed/blocked mode from UI
    connListModel()->setBlockedMode(true);

    m_connListView->setModel(connListModel());

    m_connListView->setMenu(m_btEdit->menu());
}

void ConnectionsWindow::setupTableConnListHeader()
{
    auto header = m_connListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    header->setSectionResizeMode(4, QHeaderView::Interactive);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->setSectionResizeMode(6, QHeaderView::Stretch);

    header->resizeSection(0, 430);
    header->resizeSection(1, 50);
    header->resizeSection(2, 60);
    header->resizeSection(3, 140);
    header->resizeSection(4, 140);
    header->resizeSection(5, 60);

    // header->setSectionsClickable(true);
    // header->setSortIndicatorShown(true);
    // header->setSortIndicator(4, Qt::DescendingOrder);
}

void ConnectionsWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
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
        m_actCopy->setEnabled(connSelected);
        m_actRemoveConn->setEnabled(connSelected);
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

void ConnectionsWindow::syncAutoScroll()
{
    if (settings()->connAutoScroll()) {
        connect(connListModel(), &QAbstractItemModel::rowsInserted, m_connListView,
                &QAbstractItemView::scrollToBottom);

        m_connListView->scrollToBottom();
    } else {
        disconnect(connListModel(), &QAbstractItemModel::rowsInserted, m_connListView,
                &QAbstractItemView::scrollToBottom);
    }
}

void ConnectionsWindow::syncShowHostNames()
{
    connListModel()->setResolveAddress(settings()->connShowHostNames());
}

void ConnectionsWindow::deleteConn(int row)
{
    const auto connRow = connListModel()->connRowAt(row);
    connListModel()->deleteConn(connRow.rowId, connRow.blocked, row);
}

int ConnectionsWindow::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

QString ConnectionsWindow::connListCurrentPath() const
{
    const auto connRow = connListModel()->connRowAt(connListCurrentIndex());
    return connRow.appPath;
}

FortManager *ConnectionsWindow::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *ConnectionsWindow::settings() const
{
    return ctrl()->settings();
}

FirewallConf *ConnectionsWindow::conf() const
{
    return ctrl()->conf();
}

ConnListModel *ConnectionsWindow::connListModel() const
{
    return fortManager()->connListModel();
}

AppInfoCache *ConnectionsWindow::appInfoCache() const
{
    return connListModel()->appInfoCache();
}
