#include "activitywindow.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/treeview.h>
#include <form/opt/pages/optmainpage.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/activitytreemodel.h>
#include <user/iniuser.h>
#include <util/conf/conn.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/net/netformatutil.h>
#include <util/osutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "activitycontroller.h"

using namespace Fort;

namespace {

inline constexpr int ACTIVITY_HEADER_VERSION = 4;

}

ActivityWindow::ActivityWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new ActivityController(this)), m_model(new ActivityTreeModel(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser());
}

QString ActivityWindow::iniGroup() const
{
    return IniUser::activityWindowGroup();
}

void ActivityWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    saveIniWindowGeometry();

    iniUser.setActivityHeader(m_treeView->header()->saveState());
    iniUser.setActivityHeaderVersion(ACTIVITY_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ActivityWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    restoreIniWindowGeometry(QSize(1000, 600));

    if (iniUser.activityHeaderVersion() == ACTIVITY_HEADER_VERSION) {
        m_treeView->header()->restoreState(iniUser.activityHeader());
    }
}

void ActivityWindow::showEvent(QShowEvent *event)
{
    FormWindow::showEvent(event);

    m_model->setActive(true);
}

void ActivityWindow::hideEvent(QHideEvent *event)
{
    FormWindow::hideEvent(event);

    m_model->setActive(false);
}

void ActivityWindow::setupController()
{
    connect(ctrl(), &ActivityController::retranslateUi, this, &ActivityWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void ActivityWindow::retranslateUi()
{
    this->unsetLocale();

    m_editSearch->setPlaceholderText(tr("Search") + " /");

    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actLookupIp->setText(tr("Lookup IP"));
    m_actFilterSim->setText(tr("Filter Simulator"));
    m_actFind->setText(tr("Find"));

    m_model->refreshTexts();

    this->setWindowTitle(tr("Activity"));
}

void ActivityWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Tree
    setupTreeActivity();
    setupTreeActivityMenu();
    setupTreeActivityHeader();

    // Actions on the tree's current changed
    setupTreeActivityChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addWidget(createHeaderWidget(header));
    layout->addWidget(m_treeView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 300);
}

QLayout *ActivityWindow::setupHeader()
{
    // Search
    setupEditSearch();

    // Header buttons
    m_headerButtons = new HeaderButtons(OptMainPage::TabStatistics);

    return ControlUtil::createHLayoutByWidgets(
            { m_editSearch, /*stretch*/ nullptr, m_headerButtons });
}

void ActivityWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createSearchEdit(
            [&](const QString &text) { m_model->setTextFilter(text); });

    connect(this, &ActivityWindow::aboutToShow, m_editSearch, qOverload<>(&QWidget::setFocus));
}

void ActivityWindow::setupTreeActivity()
{
    m_treeView = new TreeView();
    m_treeView->setObjectName("treeView");
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_treeView->setModel(m_model);

    // The rows are sorted on the header's click only
    m_treeView->header()->setSortIndicator(int(ActivityColumn::Program), Qt::AscendingOrder);
    m_treeView->setSortingEnabled(true);
}

void ActivityWindow::setupTreeActivityMenu()
{
    auto menu = ControlUtil::createMenu(this);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setObjectName("actCopyAsFilter");
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actLookupIp = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actLookupIp->setObjectName("actLookupIp");
    m_actLookupIp->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_L);

    m_actFilterSim = menu->addAction(IconCache::icon(":/icons/filter.png"), QString());
    m_actFilterSim->setObjectName("actFilterSim");

    menu->addSeparator();

    m_actFind = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actFind->setObjectName("actFind");
    m_actFind->setShortcut(QKeySequence::Find);

    m_treeView->setMenu(menu);

    setupTreeActivityMenuConnections();
}

void ActivityWindow::setupTreeActivityMenuConnections()
{
    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto text = ActivityTreeModel::flowsAsFilter(m_treeView->selectedRows());

        GuiUtil::setClipboardData(text);
    });
    setupFlowAction(m_actLookupIp, [](const Conn &conn) {
        OsUtil::openIpLocationUrl(NetFormatUtil::ipToText(conn.remoteIp, conn.isIPv6));
    });
    setupFlowAction(m_actFilterSim,
            [](const Conn &conn) { windowManager()->showFilterSimWindowConn(conn); });
    ControlUtil::setupFindAction(m_actFind, m_editSearch);
}

void ActivityWindow::setupFlowAction(
        QAction *action, const std::function<void(const Conn &conn)> &func)
{
    connect(action, &QAction::triggered, this, [this, func] {
        const QModelIndex index = m_treeView->currentIndex();

        if (ActivityTreeModel::isFlowIndex(index)) {
            func(ActivityTreeModel::flowConnAt(index));
        }
    });
}

void ActivityWindow::setupTreeActivityHeader()
{
    auto header = m_treeView->header();

    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);

    constexpr int columnWidths[] = { 360, 50, 50, 40, 100, 60, 120, 60, 160, 30, 30, 90, 90, 90, 90,
        40, 140 };
    static_assert(std::size(columnWidths) == int(ActivityColumn::Count));

    for (int i = 0; i < int(ActivityColumn::Count); ++i) {
        header->resizeSection(i, columnWidths[i]);
    }

    header->setSectionResizeMode(int(ActivityColumn::Direction), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ActivityColumn::Reason), QHeaderView::Fixed);

    // Hidden columns
    constexpr ActivityColumn hiddenColumns[] = {
        ActivityColumn::ProcessId,
        ActivityColumn::Protocol,
        ActivityColumn::LocalAddress,
        ActivityColumn::LocalPort,
        ActivityColumn::RemoteAddress,
        ActivityColumn::RemotePort,
        ActivityColumn::HostName,
        ActivityColumn::Direction,
        ActivityColumn::Reason,
    };

    for (const ActivityColumn column : hiddenColumns) {
        header->setSectionHidden(int(column), /*hide=*/true);
    }

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this,
            &ActivityWindow::showTreeActivityHeaderMenu);
}

void ActivityWindow::setupTreeActivityChanged()
{
    const auto refreshTreeActivityChanged = [&] {
        const bool flowSelected = ActivityTreeModel::isFlowIndex(m_treeView->currentIndex());

        m_actCopyAsFilter->setEnabled(flowSelected);
        m_actLookupIp->setEnabled(flowSelected);
        m_actFilterSim->setEnabled(flowSelected);
    };

    refreshTreeActivityChanged();

    connect(m_treeView, &TreeView::currentIndexChanged, this, refreshTreeActivityChanged);
}

void ActivityWindow::showTreeActivityHeaderMenu(const QPoint &pos)
{
    ControlUtil::showHeaderMenu(
            m_treeView->header(), pos,
            [](int column) { return ActivityTreeModel::columnName(ActivityColumn(column)); },
            tr("Stretch last column"));
}
