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
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/connblocklistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

namespace {

constexpr int CONN_LIST_HEADER_VERSION = 2;

}

ConnectionsPage::ConnectionsPage(StatisticsController *ctrl, QWidget *parent) :
    StatBasePage(ctrl, parent), m_connBlockListModel(new ConnBlockListModel(this))
{
    setupUi();

    updateAutoScroll();
    updateShowHostNames();

    connBlockListModel()->initialize();
}

AppInfoCache *ConnectionsPage::appInfoCache() const
{
    return connBlockListModel()->appInfoCache();
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
    m_actCopy->setText(tr("Copy"));
    m_actAddProgram->setText(tr("Add Program"));
    m_actRemoveConn->setText(tr("Remove"));
    m_actClearAll->setText(tr("Clear All"));

    m_btClearAll->setText(tr("Clear All"));

    m_btOptions->setText(tr("Options"));
    m_cbAutoScroll->setText(tr("Auto scroll"));
    m_cbShowHostNames->setText(tr("Show host names"));

    connBlockListModel()->refresh();

    m_appInfoRow->retranslateUi();
}

void ConnectionsPage::setupUi()
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
}

QLayout *ConnectionsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actCopy = editMenu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actAddProgram = editMenu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    m_actRemoveConn = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveConn->setShortcut(Qt::Key_Delete);

    m_actClearAll = editMenu->addAction(IconCache::icon(":/icons/recycle.png"), QString());

    connect(m_actCopy, &QAction::triggered, this,
            [&] { GuiUtil::setClipboardData(m_connListView->selectedText()); });
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto appPath = connListCurrentPath();
        if (!appPath.isEmpty()) {
            windowManager()->showProgramEditForm(appPath);
        }
    });
    connect(m_actRemoveConn, &QAction::triggered, this, [&] {
        const int connIndex = connListCurrentIndex();
        if (connIndex >= 0) {
            windowManager()->showConfirmBox([&] { deleteConn(connIndex); },
                    tr("Are you sure to remove connections till this row?"));
        }
    });
    connect(m_actClearAll, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { connBlockListModel()->clear(); },
                tr("Are you sure to remove all connections?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Toolbar buttons
    m_btClearAll = ControlUtil::createFlatToolButton(":/icons/recycle.png");

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
    m_connListView->setAlternatingRowColors(true);
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_connListView->setModel(connBlockListModel());

    m_connListView->setMenu(m_btEdit->menu());
}

void ConnectionsPage::setupTableConnListHeader()
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
        m_actCopy->setEnabled(connSelected);
        m_actAddProgram->setEnabled(connSelected);
        m_actRemoveConn->setEnabled(connSelected);
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnsChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnsChanged);
}

void ConnectionsPage::updateAutoScroll()
{
    if (iniUser()->statAutoScroll()) {
        connect(connBlockListModel(), &QAbstractItemModel::rowsInserted, m_connListView,
                &QAbstractItemView::scrollToBottom);

        m_connListView->scrollToBottom();
    } else {
        connBlockListModel()->disconnect(m_connListView);
    }
}

void ConnectionsPage::updateShowHostNames()
{
    connBlockListModel()->setResolveAddress(iniUser()->statShowHostNames());
}

void ConnectionsPage::deleteConn(int row)
{
    const auto &connRow = connBlockListModel()->connRowAt(row);
    if (connRow.isNull())
        return;

    connBlockListModel()->deleteConn(connRow.connId);
}

int ConnectionsPage::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

QString ConnectionsPage::connListCurrentPath() const
{
    const auto &connRow = connBlockListModel()->connRowAt(connListCurrentIndex());
    return connRow.isNull() ? QString() : connRow.appPath;
}
