#include "trafficwindow.h"

#include <QAction>
#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QTabBar>
#include <QTimeEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/confmanager.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/appstatmodel.h>
#include <model/traflistmodel.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "trafficcontroller.h"

using namespace Fort;

namespace {

inline constexpr int APP_LIST_HEADER_VERSION = 1;

}

TrafficWindow::TrafficWindow(QWidget *parent) :
    FormWindow(parent),
    m_ctrl(new TrafficController(this)),
    m_appStatModel(new AppStatModel(this)),
    m_trafListModel(new TrafListModel(this))
{
    setupUi();

    appStatModel()->initialize();
    trafListModel()->initialize();

    setupController();

    setupFormWindow(iniUser(), IniUser::statWindowGroup());
}

void TrafficWindow::selectTrafTab(int index)
{
    m_tabBar->setCurrentIndex(index);
}

void TrafficWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setStatWindowGeometry(stateWatcher()->geometry());
    iniUser.setStatWindowMaximized(stateWatcher()->maximized());

    // App List
    {
        auto header = m_appListView->horizontalHeader();
        iniUser.setStatAppListHeader(header->saveState());
        iniUser.setStatAppListHeaderVersion(APP_LIST_HEADER_VERSION);
    }

    iniUser.setStatWindowTrafSplit(m_splitter->saveState());

    confManager()->saveIniUser();
}

void TrafficWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(1024, 768), iniUser.statWindowGeometry(), iniUser.statWindowMaximized());

    // App List
    if (iniUser.statAppListHeaderVersion() == APP_LIST_HEADER_VERSION) {
        auto header = m_appListView->horizontalHeader();
        header->restoreState(iniUser.statAppListHeader());
    }

    // Traf Table
    {
        const int tabIndex = qBound(0, iniUser.statTrafTabIndex(), m_tabBar->count() - 1);

        m_tabBar->setCurrentIndex(tabIndex);
    }

    m_splitter->restoreState(iniUser.statWindowTrafSplit());
}

void TrafficWindow::setupController()
{
    connect(ctrl(), &TrafficController::retranslateUi, this, &TrafficWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void TrafficWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddProgram->setText(tr("Add Program"));
    m_actRemoveApp->setText(tr("Remove Application"));
    m_actResetTotal->setText(tr("Reset Total"));
    m_actClearAll->setText(tr("Clear All"));
    m_actFindApps->setText(tr("Find"));

    m_editSearch->setPlaceholderText(tr("Search") + " /");
    m_btRefresh->setText(tr("Refresh"));

    m_traphUnits->setText(tr("Units:"));
    retranslateTrafUnitNames();

    retranslateTabBar();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Traffic"));
}

void TrafficWindow::retranslateTrafUnitNames()
{
    const QStringList list = { tr("Adaptive"), tr("Bytes"), "KB", "MB", "GB", "TB", "PB" };

    ControlUtil::setComboBoxTexts(m_comboTrafUnit, list);

    updateTrafUnit();
    updateTableTrafUnit();
}

void TrafficWindow::retranslateTabBar()
{
    const QStringList list = { tr("Hourly"), tr("Daily"), tr("Monthly"), tr("Total") };

    int index = 0;
    for (const auto &v : list) {
        m_tabBar->setTabText(index++, v);
    }
}

void TrafficWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // App List
    setupAppListView();
    setupAppListHeader();

    // Tab Bar
    setupTabBar();

    // Traf Table
    setupTableTraf();
    setupTableTrafType();
    setupTableTrafApp();
    setupTableTrafTime();
    setupTableTrafHeader();

    // Splitter
    setupSplitter();

    // App Info Row
    setupAppInfoRow();

    // Actions on app list view's current changed
    setupAppListViewChanged();

    // Layout
    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_splitter, 1);
    layout->addWidget(m_appInfoRow);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *TrafficWindow::setupHeader()
{
    setupClearMenu();
    setupEditSearch();
    setupRefresh();
    setupTrafUnits();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton(3);

    // Statistics button
    m_btStatistics = ControlUtil::createStatisticsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_editSearch, ControlUtil::createVSeparator(), m_btRefresh,
            /*stretch*/ nullptr, m_traphUnits, m_comboTrafUnit, ControlUtil::createVSeparator(),
            m_btOptions, m_btStatistics, m_btMenu });

    return layout;
}

void TrafficWindow::setupClearMenu()
{
    auto menu = ControlUtil::createMenu(this);

    m_actAddProgram = menu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actRemoveApp = menu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    m_actResetTotal = menu->addAction(QString());
    m_actClearAll = menu->addAction(IconCache::icon(":/icons/broom.png"), QString());

    m_actFindApps = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actFindApps->setShortcut(QKeySequence::Find);

    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto &appStatRow = currentAppStatRow();

        if (!appStatRow.isNull()) {
            windowManager()->openProgramEditForm(appStatRow.appPath, appStatRow.confAppId, this);
        }
    });
    connect(m_actRemoveApp, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] {
                    const auto &appStatRow = currentAppStatRow();

                    if (!appStatRow.isNull()) {
                        ctrl()->deleteStatApp(appStatRow.appId);
                    }
                },
                tr("Are you sure to remove statistics for selected application?"));
    });
    connect(m_actResetTotal, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { ctrl()->resetAppTotals(); }, tr("Are you sure to reset total statistics?"));
    });
    connect(m_actClearAll, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] {
                    m_appListView->clearSelection();
                    ctrl()->clearTraffic();
                },
                tr("Are you sure to clear all statistics?"));
    });
    connect(m_actFindApps, &QAction::triggered, this, [&] { m_editSearch->setFocus(); });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(menu);
}

void TrafficWindow::setupRefresh()
{
    m_btRefresh = ControlUtil::createFlatToolButton(":/icons/arrow_refresh_small.png", [&] {
        appStatModel()->refresh();
        trafListModel()->reset();
    });
}

void TrafficWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { appStatModel()->setTextFilter(text); });
    m_editSearch->setClearButtonEnabled(true);
    m_editSearch->setMaxLength(200);
    m_editSearch->setMinimumWidth(100);
    m_editSearch->setMaximumWidth(200);
}

void TrafficWindow::setupTrafUnits()
{
    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(QStringList(), [&](int index) {
        if (iniUser().statTrafUnit() == index)
            return;

        iniUser().setStatTrafUnit(index);
        updateTableTrafUnit();

        confManager()->saveIniUser();
    });
    m_comboTrafUnit->setMinimumWidth(100);
}

void TrafficWindow::setupAppListView()
{
    m_appListView = new TableView();
    m_appListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_appListView->setSortingEnabled(true);
    m_appListView->setModel(appStatModel());

    m_appListView->setMenu(m_btEdit->menu());

    connect(m_appListView, &TableView::doubleClicked, m_actAddProgram, &QAction::trigger);
}

void TrafficWindow::setupAppListHeader()
{
    auto header = m_appListView->horizontalHeader();

    header->setSectionResizeMode(int(AppStatColumn::Program), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppStatColumn::Download), QHeaderView::Stretch);
    header->setSectionResizeMode(int(AppStatColumn::Upload), QHeaderView::Stretch);

    header->resizeSection(int(AppStatColumn::Program), 240);

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(int(AppStatColumn::Download), Qt::DescendingOrder);
}

void TrafficWindow::setupTabBar()
{
    m_tabBar = new QTabBar();
    m_tabBar->setShape(QTabBar::RoundedNorth);

    for (int n = 4; --n >= 0;) {
        m_tabBar->addTab(QString());
    }

    connect(m_tabBar, &QTabBar::tabBarDoubleClicked, this, &TrafficWindow::saveTrafTabIndex);
}

void TrafficWindow::setupTableTraf()
{
    m_tableTraf = new TableView();
    m_tableTraf->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableTraf->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_tableTraf->setModel(trafListModel());

    connect(m_tableTraf, &TableView::doubleClicked, this, [&](const QModelIndex &index) {
        m_appListView->selectRow(0);
        m_tableTraf->selectRow(index.row());
    });
}

void TrafficWindow::setupTableTrafType()
{
    updateTrafType();

    connect(m_tabBar, &QTabBar::currentChanged, this, &TrafficWindow::updateTrafType);
}

void TrafficWindow::setupTableTrafApp()
{
    updateTrafApp();

    connect(m_appListView, &TableView::currentIndexChanged, this, &TrafficWindow::updateTrafApp);
}

void TrafficWindow::setupTableTrafTime()
{
    updateAppListTime();

    connect(m_tableTraf, &TableView::currentIndexChanged, this, &TrafficWindow::updateAppListTime);
}

void TrafficWindow::setupTableTrafHeader()
{
    auto header = m_tableTraf->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    const auto refreshTableTrafHeader = [&] {
        auto hh = m_tableTraf->horizontalHeader();
        hh->resizeSection(0, qBound(150, qRound(hh->width() * 0.3f), 180));
    };

    refreshTableTrafHeader();

    connect(header, &QHeaderView::geometriesChanged, this, refreshTableTrafHeader);
}

void TrafficWindow::setupSplitter()
{
    auto layout = ControlUtil::createVLayoutByWidgets({ m_tabBar, m_tableTraf }, /*margin=*/0);

    auto trafWidget = new QWidget();
    trafWidget->setLayout(layout);

    m_splitter = new QSplitter();
    m_splitter->addWidget(m_appListView);
    m_splitter->addWidget(trafWidget);
}

void TrafficWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        const auto &appStatRow = currentAppStatRow();
        const auto appPath = appStatRow.isNull() ? QString() : appStatRow.appPath;

        m_appInfoRow->refreshAppInfoVersion(appPath, appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void TrafficWindow::setupAppListViewChanged()
{
    const auto refreshAppListViewChanged = [&] {
        const bool appSelected = (appListCurrentIndex() > 0);
        m_actAddProgram->setEnabled(appSelected);
        m_actRemoveApp->setEnabled(appSelected);
        m_appInfoRow->setVisible(appSelected);
    };

    refreshAppListViewChanged();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshAppListViewChanged);
}

void TrafficWindow::updateTrafType()
{
    const auto trafType = TrafUnitType::TrafType(m_tabBar->currentIndex());

    appStatModel()->setType(trafType);
    appStatModel()->setTrafTime(-1);

    trafListModel()->setType(trafType);
}

void TrafficWindow::updateTrafApp(const QModelIndex &index)
{
    if (!index.isValid()) {
        m_appListView->selectRow(0);
        return;
    }

    const auto &appStatRow = currentAppStatRow();
    const qint64 appId = appStatRow.isNull() ? 0 : appStatRow.appId;

    const bool oldHasApp = trafListModel()->hasApp();

    trafListModel()->setAppId(appId);

    if (oldHasApp && !trafListModel()->hasApp()) {
        updateTrafType();
        updateAppListTime();
    }
}

void TrafficWindow::updateAppListTime(const QModelIndex &index)
{
    if (!index.isValid()) {
        const int row =
                trafListModel()->rowByTime(appStatModel()->trafTime(), appStatModel()->type());

        if (row >= 0) {
            m_tableTraf->selectRow(row);
            return;
        }
    }

    if (trafListModel()->hasApp())
        return;

    const auto &trafficRow = trafListModel()->trafficRowAt(tableTrafCurrentIndex());
    const qint32 trafTime =
            trafficRow.isNull() ? trafListModel()->maxTrafTime() : trafficRow.trafTime;

    appStatModel()->setTrafTime(trafTime);
}

void TrafficWindow::updateTrafUnit()
{
    m_comboTrafUnit->setCurrentIndex(iniUser().statTrafUnit());
}

void TrafficWindow::updateTableTrafUnit()
{
    const auto trafUnit = TrafUnitType::TrafUnit(iniUser().statTrafUnit());

    appStatModel()->setUnit(trafUnit);
    trafListModel()->setUnit(trafUnit);
}

int TrafficWindow::appListCurrentIndex() const
{
    return m_appListView->currentRow();
}

const AppStatRow &TrafficWindow::currentAppStatRow() const
{
    return appStatModel()->appStatRowAt(appListCurrentIndex());
}

int TrafficWindow::tableTrafCurrentIndex() const
{
    return m_tableTraf->currentRow();
}

void TrafficWindow::saveTrafTabIndex(int tabIndex)
{
    if (QGuiApplication::keyboardModifiers() != Qt::ControlModifier)
        return;

    windowManager()->showConfirmBox(
            [=, this] { iniUser().setValue(IniUser::statTrafTabIndexKey(), tabIndex); },
            tr("Make this tab active when window opens?"));
}
