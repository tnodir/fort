#include "statisticswindow.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../appinfo/appinfocache.h"
#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/connlistmodel.h"
#include "../../user/iniuser.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../../util/window/widgetwindowstatewatcher.h"
#include "../controls/appinforow.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "statisticscontroller.h"

namespace {

#define CONN_LIST_HEADER_VERSION 1

}

StatisticsWindow::StatisticsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new StatisticsController(fortManager, this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();

    syncAutoScroll();
    syncShowHostNames();
}

FortManager *StatisticsWindow::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *StatisticsWindow::settings() const
{
    return ctrl()->settings();
}

ConfManager *StatisticsWindow::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *StatisticsWindow::conf() const
{
    return ctrl()->conf();
}

IniOptions *StatisticsWindow::ini() const
{
    return ctrl()->ini();
}

IniUser *StatisticsWindow::iniUser() const
{
    return ctrl()->iniUser();
}

ConnListModel *StatisticsWindow::connListModel() const
{
    return fortManager()->connListModel();
}

AppInfoCache *StatisticsWindow::appInfoCache() const
{
    return connListModel()->appInfoCache();
}

void StatisticsWindow::saveWindowState()
{
    iniUser()->setStatWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setStatWindowMaximized(m_stateWatcher->maximized());

    auto header = m_connListView->horizontalHeader();
    iniUser()->setConnListHeader(header->saveState());
    iniUser()->setConnListHeaderVersion(CONN_LIST_HEADER_VERSION);

    confManager()->saveIniUser();
}

void StatisticsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), iniUser()->statWindowGeometry(),
            iniUser()->statWindowMaximized());

    if (iniUser()->connListHeaderVersion() == CONN_LIST_HEADER_VERSION) {
        auto header = m_connListView->horizontalHeader();
        header->restoreState(iniUser()->connListHeader());
    }
}

void StatisticsWindow::setupController()
{
    connect(ctrl(), &StatisticsController::retranslateUi, this, &StatisticsWindow::retranslateUi);

    retranslateUi();
}

void StatisticsWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void StatisticsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actCopy->setText(tr("Copy"));
    m_actAddProgram->setText(tr("Add Program"));
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

void StatisticsWindow::setupUi()
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
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/connect.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *StatisticsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actCopy = editMenu->addAction(IconCache::icon(":/icons/copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actAddProgram = editMenu->addAction(IconCache::icon(":/icons/window.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    m_actRemoveConn = editMenu->addAction(IconCache::icon(":/icons/sign-delete.png"), QString());
    m_actRemoveConn->setShortcut(Qt::Key_Delete);

    m_actClearConns = editMenu->addAction(IconCache::icon(":/icons/trashcan-full.png"), QString());

    connect(m_actCopy, &QAction::triggered, this,
            [&] { GuiUtil::setClipboardData(m_connListView->selectedText()); });
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto connIndex = connListCurrentIndex();
        const auto connRow = connListModel()->connRowAt(connIndex);

        fortManager()->showProgramEditForm(connRow.appPath);
    });
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

void StatisticsWindow::setupLogOptions()
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

void StatisticsWindow::setupLogAllowedIp()
{
    m_cbLogAllowedIp = ControlUtil::createCheckBox(conf()->logAllowedIp(), [&](bool checked) {
        if (conf()->logAllowedIp() == checked)
            return;

        conf()->setLogAllowedIp(checked);

        confManager()->saveFlags();
    });

    m_cbLogAllowedIp->setVisible(false); // TODO: Collect allowed connections
}

void StatisticsWindow::setupLogBlockedIp()
{
    m_cbLogBlockedIp = ControlUtil::createCheckBox(conf()->logBlockedIp(), [&](bool checked) {
        if (conf()->logBlockedIp() == checked)
            return;

        conf()->setLogBlockedIp(checked);

        confManager()->saveFlags();
    });
}

void StatisticsWindow::setupAutoScroll()
{
    m_cbAutoScroll = ControlUtil::createCheckBox(iniUser()->connAutoScroll(), [&](bool checked) {
        if (iniUser()->connAutoScroll() == checked)
            return;

        iniUser()->setConnAutoScroll(checked);
        confManager()->saveIniUser();

        syncAutoScroll();
    });
}

void StatisticsWindow::setupShowHostNames()
{
    m_cbShowHostNames =
            ControlUtil::createCheckBox(iniUser()->connShowHostNames(), [&](bool checked) {
                if (iniUser()->connShowHostNames() == checked)
                    return;

                iniUser()->setConnShowHostNames(checked);
                confManager()->saveIniUser();

                syncShowHostNames();
            });
}

void StatisticsWindow::setupTableConnList()
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

void StatisticsWindow::setupTableConnListHeader()
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

void StatisticsWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void StatisticsWindow::setupTableConnsChanged()
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

void StatisticsWindow::syncAutoScroll()
{
    if (iniUser()->connAutoScroll()) {
        connect(connListModel(), &QAbstractItemModel::rowsInserted, m_connListView,
                &QAbstractItemView::scrollToBottom);

        m_connListView->scrollToBottom();
    } else {
        disconnect(connListModel(), &QAbstractItemModel::rowsInserted, m_connListView,
                &QAbstractItemView::scrollToBottom);
    }
}

void StatisticsWindow::syncShowHostNames()
{
    connListModel()->setResolveAddress(iniUser()->connShowHostNames());
}

void StatisticsWindow::deleteConn(int row)
{
    const auto connRow = connListModel()->connRowAt(row);
    connListModel()->deleteConn(connRow.rowId, connRow.blocked, row);
}

int StatisticsWindow::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

QString StatisticsWindow::connListCurrentPath() const
{
    const auto connRow = connListModel()->connRowAt(connListCurrentIndex());
    return connRow.appPath;
}
