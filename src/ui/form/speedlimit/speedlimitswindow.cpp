#include "speedlimitswindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/progressitemdelegate.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/speedlimitlistmodel.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "speedlimiteditdialog.h"
#include "speedlimitscontroller.h"

using namespace Fort;

namespace {

inline constexpr int SPEED_LIMITS_HEADER_VERSION = 2;

inline constexpr int SPEED_LIMIT_STATUS_INTERVAL_MSEC = 500;

}

SpeedLimitsWindow::SpeedLimitsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new SpeedLimitsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::speedLimitWindowGroup());
}

void SpeedLimitsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setSpeedLimitWindowGeometry(stateWatcher()->geometry());
    iniUser.setSpeedLimitWindowMaximized(stateWatcher()->maximized());

    auto header = m_speedLimitListView->horizontalHeader();
    iniUser.setSpeedLimitsHeader(header->saveState());
    iniUser.setSpeedLimitsHeaderVersion(SPEED_LIMITS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void SpeedLimitsWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(this, QSize(700, 600), iniUser.speedLimitWindowGeometry(),
            iniUser.speedLimitWindowMaximized());

    if (iniUser.speedLimitsHeaderVersion() == SPEED_LIMITS_HEADER_VERSION) {
        auto header = m_speedLimitListView->horizontalHeader();
        header->restoreState(iniUser.speedLimitsHeader());
    }
}

void SpeedLimitsWindow::setupController()
{
    connect(ctrl(), &SpeedLimitsController::retranslateUi, this, &SpeedLimitsWindow::retranslateUi);

    emit ctrl() -> retranslateUi();
}

void SpeedLimitsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddSpeedLimit->setText(tr("Add"));
    m_actEditSpeedLimit->setText(tr("Edit"));
    m_actRemoveSpeedLimit->setText(tr("Remove"));

    speedLimitListModel()->refresh();

    this->setWindowTitle(tr("Speed Limits"));
}

void SpeedLimitsWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableSpeedLimits();
    setupTableSpeedLimitsHeader();

    // Actions on speed limits table's current changed
    setupTableSpeedLimitsChanged();

    // Actions on speed limit list model's changed
    setupSpeedLimitListModelChanged();

    // Queues' status of the driver
    setupStatusTimer();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_speedLimitListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 300);
}

QLayout *SpeedLimitsWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddSpeedLimit = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddSpeedLimit->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actEditSpeedLimit = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditSpeedLimit->setShortcut(Qt::Key_Enter);

    m_actRemoveSpeedLimit = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveSpeedLimit->setShortcut(Qt::Key_Delete);

    connect(m_actAddSpeedLimit, &QAction::triggered, this, &SpeedLimitsWindow::addNewSpeedLimit);
    connect(m_actEditSpeedLimit, &QAction::triggered, this,
            &SpeedLimitsWindow::editSelectedSpeedLimit);
    connect(m_actRemoveSpeedLimit, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { deleteSelectedSpeedLimit(); },
                tr("Are you sure to remove selected speed limit?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btEdit, /*stretch*/ nullptr, m_btOptions, m_btMenu });

    return layout;
}

void SpeedLimitsWindow::setupTableSpeedLimits()
{
    m_speedLimitListView = new TableView();
    m_speedLimitListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_speedLimitListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_speedLimitListView->setModel(speedLimitListModel());

    m_speedLimitListView->setItemDelegateForColumn(
            int(SpeedLimitListColumn::Queue), new ProgressItemDelegate(m_speedLimitListView));

    m_speedLimitListView->setMenu(m_btEdit->menu());

    connect(m_speedLimitListView, &TableView::activated, m_actEditSpeedLimit, &QAction::trigger);
}

void SpeedLimitsWindow::setupTableSpeedLimitsHeader()
{
    auto header = m_speedLimitListView->horizontalHeader();

    header->setSectionResizeMode(int(SpeedLimitListColumn::Name), QHeaderView::Interactive);
    header->setSectionResizeMode(int(SpeedLimitListColumn::Queue), QHeaderView::Interactive);
    header->setSectionResizeMode(int(SpeedLimitListColumn::Dropped), QHeaderView::Interactive);
    header->setSectionResizeMode(int(SpeedLimitListColumn::Direction), QHeaderView::Fixed);
    header->setSectionResizeMode(int(SpeedLimitListColumn::ModTime), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(SpeedLimitListColumn::Name), 300);
    header->resizeSection(int(SpeedLimitListColumn::Queue), 140);
    header->resizeSection(int(SpeedLimitListColumn::Dropped), 70);
    header->resizeSection(int(SpeedLimitListColumn::Direction), 30);
    header->resizeSection(int(SpeedLimitListColumn::ModTime), 130);
}

void SpeedLimitsWindow::setupTableSpeedLimitsChanged()
{
    const auto refreshTableSpeedLimitsChanged = [&] {
        const int speedLimitIndex = speedLimitListCurrentIndex();
        const bool speedLimitSelected = (speedLimitIndex >= 0);
        m_actEditSpeedLimit->setEnabled(speedLimitSelected);
        m_actRemoveSpeedLimit->setEnabled(speedLimitSelected);
    };

    refreshTableSpeedLimitsChanged();

    connect(m_speedLimitListView, &TableView::currentIndexChanged, this,
            refreshTableSpeedLimitsChanged);
}

void SpeedLimitsWindow::setupSpeedLimitListModelChanged()
{
    const auto refreshAddSpeedLimit = [&] {
        m_actAddSpeedLimit->setEnabled(
                speedLimitListModel()->rowCount() < ConfUtil::speedLimitMaxCount());
    };

    refreshAddSpeedLimit();

    connect(speedLimitListModel(), &SpeedLimitListModel::modelReset, this, refreshAddSpeedLimit);
    connect(speedLimitListModel(), &SpeedLimitListModel::rowsRemoved, this, refreshAddSpeedLimit);
}

void SpeedLimitsWindow::setupStatusTimer()
{
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(SPEED_LIMIT_STATUS_INTERVAL_MSEC);

    connect(m_statusTimer, &QTimer::timeout, ctrl(), &SpeedLimitsController::readSpeedLimitStatus);

    connect(this, &SpeedLimitsWindow::visibilityChanged, this,
            &SpeedLimitsWindow::updateStatusTimer);
}

void SpeedLimitsWindow::updateStatusTimer()
{
    // The minimized window keeps visible
    const bool isActive = isVisible() && !isMinimized();

    if (isActive == m_statusTimer->isActive())
        return;

    if (isActive) {
        ctrl()->readSpeedLimitStatus();
        m_statusTimer->start();
    } else {
        m_statusTimer->stop();
        ctrl()->clearSpeedLimitStatus();
    }
}

void SpeedLimitsWindow::addNewSpeedLimit()
{
    openSpeedLimitEditForm({});
}

void SpeedLimitsWindow::editSelectedSpeedLimit()
{
    const int speedLimitIndex = speedLimitListCurrentIndex();
    if (speedLimitIndex < 0)
        return;

    const auto &speedLimitRow = speedLimitListModel()->speedLimitRowAt(speedLimitIndex);

    openSpeedLimitEditForm(speedLimitRow);
}

void SpeedLimitsWindow::openSpeedLimitEditForm(const SpeedLimitRow &speedLimitRow)
{
    if (!m_formSpeedLimitEdit) {
        m_formSpeedLimitEdit = new SpeedLimitEditDialog(ctrl(), this);
    }

    m_formSpeedLimitEdit->initialize(speedLimitRow);

    WidgetWindow::showWidget(m_formSpeedLimitEdit);
}

void SpeedLimitsWindow::deleteSpeedLimit(int row)
{
    const auto &speedLimitRow = speedLimitListModel()->speedLimitRowAt(row);
    if (speedLimitRow.isNull())
        return;

    ctrl()->deleteSpeedLimit(speedLimitRow.limitId);
}

void SpeedLimitsWindow::deleteSelectedSpeedLimit()
{
    deleteSpeedLimit(speedLimitListCurrentIndex());
}

int SpeedLimitsWindow::speedLimitListCurrentIndex() const
{
    return m_speedLimitListView->currentRow();
}
