#include "timeperiodswindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/timeperiodlistmodel.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "timeperiodeditdialog.h"
#include "timeperiodscontroller.h"

using namespace Fort;

namespace {

inline constexpr int TIME_PERIODS_HEADER_VERSION = 1;

}

TimePeriodsWindow::TimePeriodsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new TimePeriodsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::timePeriodWindowGroup());
}

void TimePeriodsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setTimePeriodWindowGeometry(stateWatcher()->geometry());
    iniUser.setTimePeriodWindowMaximized(stateWatcher()->maximized());

    auto header = m_timePeriodListView->horizontalHeader();
    iniUser.setTimePeriodsHeader(header->saveState());
    iniUser.setTimePeriodsHeaderVersion(TIME_PERIODS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void TimePeriodsWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(this, QSize(700, 500), iniUser.timePeriodWindowGeometry(),
            iniUser.timePeriodWindowMaximized());

    if (iniUser.timePeriodsHeaderVersion() == TIME_PERIODS_HEADER_VERSION) {
        auto header = m_timePeriodListView->horizontalHeader();
        header->restoreState(iniUser.timePeriodsHeader());
    }
}

void TimePeriodsWindow::setupController()
{
    connect(ctrl(), &TimePeriodsController::retranslateUi, this, &TimePeriodsWindow::retranslateUi);

    emit ctrl() -> retranslateUi();
}

void TimePeriodsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddTimePeriod->setText(tr("Add"));
    m_actEditTimePeriod->setText(tr("Edit"));
    m_actRemoveTimePeriod->setText(tr("Remove"));

    timePeriodListModel()->refresh();

    this->setWindowTitle(tr("Time Periods"));
}

void TimePeriodsWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableTimePeriods();
    setupTableTimePeriodsHeader();

    // Actions on time periods table's current changed
    setupTableTimePeriodsChanged();

    // Actions on time period list model's changed
    setupTimePeriodListModelChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_timePeriodListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 300);
}

QLayout *TimePeriodsWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddTimePeriod = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddTimePeriod->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actEditTimePeriod = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditTimePeriod->setShortcut(Qt::Key_Enter);

    m_actRemoveTimePeriod = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveTimePeriod->setShortcut(Qt::Key_Delete);

    connect(m_actAddTimePeriod, &QAction::triggered, this, &TimePeriodsWindow::addNewTimePeriod);
    connect(m_actEditTimePeriod, &QAction::triggered, this,
            &TimePeriodsWindow::editSelectedTimePeriod);
    connect(m_actRemoveTimePeriod, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { deleteSelectedTimePeriod(); },
                tr("Are you sure to remove selected time period?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Statistics button
    m_btStatistics = ControlUtil::createStatisticsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btEdit, /*stretch*/ nullptr, m_btOptions, m_btStatistics, m_btMenu });

    return layout;
}

void TimePeriodsWindow::setupTableTimePeriods()
{
    m_timePeriodListView = new TableView();
    m_timePeriodListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_timePeriodListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_timePeriodListView->setModel(timePeriodListModel());

    m_timePeriodListView->setMenu(m_btEdit->menu());

    connect(m_timePeriodListView, &TableView::activated, m_actEditTimePeriod, &QAction::trigger);
}

void TimePeriodsWindow::setupTableTimePeriodsHeader()
{
    auto header = m_timePeriodListView->horizontalHeader();

    header->setSectionResizeMode(int(TimePeriodListColumn::Name), QHeaderView::Interactive);
    header->setSectionResizeMode(int(TimePeriodListColumn::Intervals), QHeaderView::Interactive);
    header->setSectionResizeMode(int(TimePeriodListColumn::ModTime), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(TimePeriodListColumn::Name), 200);
    header->resizeSection(int(TimePeriodListColumn::Intervals), 300);
    header->resizeSection(int(TimePeriodListColumn::ModTime), 130);
}

void TimePeriodsWindow::setupTableTimePeriodsChanged()
{
    const auto refreshTableTimePeriodsChanged = [&] {
        const int timePeriodIndex = timePeriodListCurrentIndex();
        const bool timePeriodSelected = (timePeriodIndex >= 0);
        m_actEditTimePeriod->setEnabled(timePeriodSelected);
        m_actRemoveTimePeriod->setEnabled(timePeriodSelected);
    };

    refreshTableTimePeriodsChanged();

    connect(m_timePeriodListView, &TableView::currentIndexChanged, this,
            refreshTableTimePeriodsChanged);
}

void TimePeriodsWindow::setupTimePeriodListModelChanged()
{
    const auto refreshAddTimePeriod = [&] {
        m_actAddTimePeriod->setEnabled(
                timePeriodListModel()->rowCount() < ConfUtil::timePeriodMaxCount());
    };

    refreshAddTimePeriod();

    connect(timePeriodListModel(), &TimePeriodListModel::modelReset, this, refreshAddTimePeriod);
    connect(timePeriodListModel(), &TimePeriodListModel::rowsRemoved, this, refreshAddTimePeriod);
}

void TimePeriodsWindow::addNewTimePeriod()
{
    openTimePeriodEditForm({});
}

void TimePeriodsWindow::editSelectedTimePeriod()
{
    const int timePeriodIndex = timePeriodListCurrentIndex();
    if (timePeriodIndex < 0)
        return;

    const auto &timePeriodRow = timePeriodListModel()->timePeriodRowAt(timePeriodIndex);

    openTimePeriodEditForm(timePeriodRow);
}

void TimePeriodsWindow::openTimePeriodEditForm(const TimePeriodRow &timePeriodRow)
{
    if (!m_formTimePeriodEdit) {
        m_formTimePeriodEdit = new TimePeriodEditDialog(ctrl(), this);
    }

    m_formTimePeriodEdit->initialize(timePeriodRow);

    WidgetWindow::showWidget(m_formTimePeriodEdit);
}

void TimePeriodsWindow::deleteTimePeriod(int row)
{
    const auto &timePeriodRow = timePeriodListModel()->timePeriodRowAt(row);
    if (timePeriodRow.isNull())
        return;

    ctrl()->deleteTimePeriod(timePeriodRow.periodId);
}

void TimePeriodsWindow::deleteSelectedTimePeriod()
{
    deleteTimePeriod(timePeriodListCurrentIndex());
}

int TimePeriodsWindow::timePeriodListCurrentIndex() const
{
    return m_timePeriodListView->currentRow();
}
