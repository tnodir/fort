#include "programswindow.h"

#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/tray/trayicon.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/applistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "programeditdialog.h"
#include "programscontroller.h"

namespace {

constexpr int APPS_HEADER_VERSION = 12;

QToolButton *createCheckableToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = ControlUtil::createFlatToolButton(iconPath, onClicked);

    c->setCheckable(true);
    c->setChecked(true);

    return c;
}

void setToolButtonCount(QToolButton *c, int count)
{
    const bool isEmpty = (count == 0);
    const auto text = isEmpty ? QString() : QString::number(count);

    c->setText(text);
    c->setVisible(!isEmpty);
}

}

ProgramsWindow::ProgramsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new ProgramsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::progWindowGroup());
}

FortSettings *ProgramsWindow::settings() const
{
    return ctrl()->settings();
}

ConfManager *ProgramsWindow::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *ProgramsWindow::conf() const
{
    return ctrl()->conf();
}

IniOptions *ProgramsWindow::ini() const
{
    return ctrl()->ini();
}

IniUser *ProgramsWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *ProgramsWindow::windowManager() const
{
    return ctrl()->windowManager();
}

AppInfoCache *ProgramsWindow::appInfoCache() const
{
    return ctrl()->appInfoCache();
}

AppListModel *ProgramsWindow::appListModel() const
{
    return ctrl()->appListModel();
}

void ProgramsWindow::saveWindowState(bool /*wasVisible*/)
{
    if (iniUser()->progWindowAutoClearAlerts()) {
        ctrl()->clearAlerts();
    }

    iniUser()->setProgWindowGeometry(stateWatcher()->geometry());
    iniUser()->setProgWindowMaximized(stateWatcher()->maximized());

    auto header = m_appListView->horizontalHeader();
    iniUser()->setProgAppsHeader(header->saveState());
    iniUser()->setProgAppsHeaderVersion(APPS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ProgramsWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(1024, 768), iniUser()->progWindowGeometry(),
            iniUser()->progWindowMaximized());

    if (iniUser()->progAppsHeaderVersion() == APPS_HEADER_VERSION) {
        auto header = m_appListView->horizontalHeader();
        header->restoreState(iniUser()->progAppsHeader());
    }
}

void ProgramsWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramsWindow::retranslateUi);

    retranslateUi();
}

void ProgramsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAllowApp->setText(tr("Allow"));
    m_actBlockApp->setText(tr("Block"));
    m_actKillApp->setText(tr("Kill Process"));
    m_actAddApp->setText(tr("Add"));
    m_actAddWildcard->setText(tr("Add Wildcard"));
    m_actToWildcard->setText(tr("Convert to Wildcard"));
    m_actEditApp->setText(tr("Edit"));
    m_actRemoveApp->setText(tr("Remove"));
    m_actReviewAlerts->setText(tr("Review Alerts"));
    m_actClearAlerts->setText(tr("Clear Alerts"));
    m_actPurgeApps->setText(tr("Purge Obsolete"));
    m_actFindApps->setText(tr("Find"));

    m_btAllowApp->setText(tr("Allow"));
    m_btBlockApp->setText(tr("Block"));
    m_btRemoveApp->setText(tr("Remove"));
    m_editSearch->setPlaceholderText(tr("Search"));
    m_btFilter->setToolTip(tr("Filters"));
    m_btClearFilter->setText(tr("Clear Filters"));
    m_cbFilterAlerted->setText(tr("Alerts"));
    m_cbFilterWildcard->setText(tr("Wildcard Paths"));
    m_cbFilterParked->setText(tr("Parked"));
    m_cbFilterKillProcess->setText(tr("Kill Process"));

    appListModel()->refresh();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Programs"));
}

void ProgramsWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableApps();
    setupTableAppsHeader();

    // App Info Row
    setupAppInfoRow();

    // Actions on apps table's current changed
    setupTableAppsChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_appListView, 1);
    layout->addWidget(m_appInfoRow);

    this->setLayout(layout);

    // Accept File Drops
    setAcceptDrops(true);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(800, 400);
}

QLayout *ProgramsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    setupEditMenu();

    // Toolbar buttons
    m_btAllowApp = ControlUtil::createFlatToolButton(":/icons/accept.png");
    m_btBlockApp = ControlUtil::createFlatToolButton(":/icons/deny.png");
    m_btRemoveApp = ControlUtil::createFlatToolButton(":/icons/delete.png");

    connect(m_btAllowApp, &QAbstractButton::clicked, m_actAllowApp, &QAction::trigger);
    connect(m_btBlockApp, &QAbstractButton::clicked, m_actBlockApp, &QAction::trigger);
    connect(m_btRemoveApp, &QAbstractButton::clicked, m_actRemoveApp, &QAction::trigger);

    // Search edit line
    setupEditSearch();

    // Filter button
    setupFilter();

    // Sort States
    auto sortStatesLayout = setupSortStatesLayout();
    updateSortStateCounts();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btAllowApp);
    layout->addWidget(m_btBlockApp);
    layout->addWidget(m_btRemoveApp);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_editSearch);
    layout->addWidget(m_btFilter);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addLayout(sortStatesLayout);
    layout->addStretch();
    layout->addWidget(m_btOptions);
    layout->addWidget(m_btMenu);

    return layout;
}

void ProgramsWindow::setupEditMenu()
{
    auto editMenu = ControlUtil::createMenu(this);

    m_actAllowApp = editMenu->addAction(IconCache::icon(":/icons/accept.png"), QString());
    m_actAllowApp->setShortcut(Qt::Key_A);

    m_actBlockApp = editMenu->addAction(IconCache::icon(":/icons/deny.png"), QString());
    m_actBlockApp->setShortcut(Qt::Key_B);

    m_actKillApp = editMenu->addAction(IconCache::icon(":/icons/scull.png"), QString());
    m_actKillApp->setShortcut(QKeyCombination(Qt::CTRL | Qt::ALT, Qt::Key_K));

    editMenu->addSeparator();

    m_actAddApp = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddApp->setShortcut(Qt::Key_Plus);

    m_actAddWildcard = editMenu->addAction(IconCache::icon(":/icons/coding.png"), QString());
    m_actAddWildcard->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actToWildcard = editMenu->addAction(IconCache::icon(":/icons/coding.png"), QString());

    m_actEditApp = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditApp->setShortcut(Qt::Key_Enter);

    m_actRemoveApp = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    editMenu->addSeparator();

    m_actReviewAlerts = editMenu->addAction(IconCache::icon(":/icons/error.png"), QString());
    m_actClearAlerts = editMenu->addAction(QString());

    editMenu->addSeparator();

    m_actPurgeApps = editMenu->addAction(IconCache::icon(":/icons/recycle.png"), QString());
    m_actPurgeApps->setShortcut(QKeyCombination(Qt::CTRL | Qt::ALT, Qt::Key_P));

    m_actFindApps = editMenu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actFindApps->setShortcut(QKeySequence::Find);

    connect(m_actAllowApp, &QAction::triggered, this,
            [&] { updateSelectedApps(/*blocked=*/false); });
    connect(m_actBlockApp, &QAction::triggered, this,
            [&] { updateSelectedApps(/*blocked=*/true); });
    connect(m_actKillApp, &QAction::triggered, this,
            [&] { updateSelectedApps(/*blocked=*/true, /*killProcess=*/true); });
    connect(m_actAddApp, &QAction::triggered, this, &ProgramsWindow::addNewProgram);
    connect(m_actAddWildcard, &QAction::triggered, this, &ProgramsWindow::addNewWildcard);
    connect(m_actToWildcard, &QAction::triggered, this, &ProgramsWindow::convertToWildcard);
    connect(m_actEditApp, &QAction::triggered, this, &ProgramsWindow::editSelectedPrograms);
    connect(m_actRemoveApp, &QAction::triggered, this, &ProgramsWindow::deleteSelectedApps);
    connect(m_actReviewAlerts, &QAction::triggered, this,
            [&] { windowManager()->showProgramAlertWindow(); });
    connect(m_actClearAlerts, &QAction::triggered, this, &ProgramsWindow::clearAlerts);
    connect(m_actPurgeApps, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox([&] { ctrl()->purgeApps(); },
                tr("Are you sure to remove all non-existent programs?"));
    });
    connect(m_actFindApps, &QAction::triggered, this, [&] { m_editSearch->setFocus(); });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);
}

void ProgramsWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { appListModel()->setFtsFilter(text); });
    m_editSearch->setClearButtonEnabled(true);
    m_editSearch->setMaxLength(200);
    m_editSearch->setMinimumWidth(100);
    m_editSearch->setMaximumWidth(200);

    connect(this, &ProgramsWindow::aboutToShow, m_editSearch, qOverload<>(&QWidget::setFocus));
}

void ProgramsWindow::setupFilter()
{
    setupFilterCheckBoxes();
    setupFilterClear();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterAlerted);
    layout->addWidget(m_cbFilterWildcard);
    layout->addWidget(m_cbFilterParked);
    layout->addWidget(m_cbFilterKillProcess);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_btClearFilter, 0, Qt::AlignCenter);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btFilter = ControlUtil::createButton(":/icons/filter.png");
    m_btFilter->setMenu(menu);

    const auto refreshFilter = [&] {
        const auto isEmpty = (appListModel()->filters() == AppListModel::FilterNone);

        m_btFilter->setIcon(isEmpty
                        ? IconCache::icon(":/icons/filter.png")
                        : GuiUtil::overlayIcon(":/icons/filter.png", ":/icons/tick.png"));
    };

    refreshFilter();

    connect(appListModel(), &AppListModel::filtersChanged, this, refreshFilter);
}

void ProgramsWindow::setupFilterCheckBoxes()
{
    m_cbFilterAlerted =
            ControlUtil::createCheckStateBox(":/icons/error.png", Qt::PartiallyChecked, [&] {
                appListModel()->setFilterValue(
                        AppListModel::FilterAlerted, m_cbFilterAlerted->checkState());
            });

    m_cbFilterWildcard =
            ControlUtil::createCheckStateBox(":/icons/coding.png", Qt::PartiallyChecked, [&] {
                appListModel()->setFilterValue(
                        AppListModel::FilterWildcard, m_cbFilterWildcard->checkState());
            });

    m_cbFilterParked =
            ControlUtil::createCheckStateBox(":/icons/parking.png", Qt::PartiallyChecked, [&] {
                appListModel()->setFilterValue(
                        AppListModel::FilterParked, m_cbFilterParked->checkState());
            });

    m_cbFilterKillProcess =
            ControlUtil::createCheckStateBox(":/icons/scull.png", Qt::PartiallyChecked, [&] {
                appListModel()->setFilterValue(
                        AppListModel::FilterKillProcess, m_cbFilterKillProcess->checkState());
            });
}

void ProgramsWindow::setupFilterClear()
{
    m_btClearFilter = ControlUtil::createFlatToolButton(":/icons/broom.png", [&] {
        appListModel()->clearFilters();

        m_cbFilterAlerted->setCheckState(Qt::PartiallyChecked);
        m_cbFilterWildcard->setCheckState(Qt::PartiallyChecked);
        m_cbFilterParked->setCheckState(Qt::PartiallyChecked);
        m_cbFilterKillProcess->setCheckState(Qt::PartiallyChecked);
    });
}

QLayout *ProgramsWindow::setupSortStatesLayout()
{
    m_btSortAllowed = createCheckableToolButton(
            ":/icons/accept.png", [&] { onSortStateClicked(AppListModel::SortAllowed); });
    m_btSortBlocked = createCheckableToolButton(
            ":/icons/deny.png", [&] { onSortStateClicked(AppListModel::SortBlocked); });
    m_btSortAlerted = createCheckableToolButton(
            ":/icons/error.png", [&] { onSortStateClicked(AppListModel::SortAlerted); });

    appListModel()->setSortState(qBound(AppListModel::SortNone,
            AppListModel::SortState(iniUser()->progWindowSortState()), AppListModel::SortAlerted));

    const auto refreshSortStates = [&] {
        const auto sortState = appListModel()->sortState();

        m_btSortAllowed->setChecked(sortState == AppListModel::SortAllowed);
        m_btSortBlocked->setChecked(sortState == AppListModel::SortBlocked);
        m_btSortAlerted->setChecked(sortState == AppListModel::SortAlerted);
    };

    refreshSortStates();

    connect(appListModel(), &AppListModel::sortStateChanged, this, refreshSortStates);

    // Layout
    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btSortAllowed, m_btSortBlocked, m_btSortAlerted });
    layout->setSpacing(0);

    return layout;
}

void ProgramsWindow::setupTableApps()
{
    m_appListView = new TableView();
    m_appListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_appListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_appListView->setSortingEnabled(true);
    m_appListView->setModel(appListModel());

    m_appListView->setMenu(m_btEdit->menu());

    connect(m_appListView, &TableView::activated, m_actEditApp, &QAction::trigger);
}

void ProgramsWindow::setupTableAppsHeader()
{
    auto header = m_appListView->horizontalHeader();

    header->setSectionResizeMode(int(AppListColumn::Name), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppListColumn::Zones), QHeaderView::Fixed);
    header->setSectionResizeMode(int(AppListColumn::Rule), QHeaderView::Fixed);
    header->setSectionResizeMode(int(AppListColumn::Scheduled), QHeaderView::Fixed);
    header->setSectionResizeMode(int(AppListColumn::Action), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppListColumn::Group), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppListColumn::FilePath), QHeaderView::Interactive);
    header->setSectionResizeMode(int(AppListColumn::CreationTime), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(AppListColumn::Name), 300);
    header->resizeSection(int(AppListColumn::Zones), 30);
    header->resizeSection(int(AppListColumn::Rule), 30);
    header->resizeSection(int(AppListColumn::Scheduled), 30);
    header->resizeSection(int(AppListColumn::Action), 100);
    header->resizeSection(int(AppListColumn::Group), 100);
    header->resizeSection(int(AppListColumn::FilePath), 270);
    header->resizeSection(int(AppListColumn::CreationTime), 130);

    header->setSectionsMovable(true);
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(int(AppListColumn::CreationTime), Qt::DescendingOrder);

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this,
            &ProgramsWindow::showTableAppsHeaderMenu);
}

void ProgramsWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(appListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void ProgramsWindow::setupTableAppsChanged()
{
    const auto refreshTableAppsChanged = [&] {
        const int appIndex = appListCurrentIndex();
        const bool appSelected = (appIndex >= 0);
        m_actAllowApp->setEnabled(appSelected);
        m_actBlockApp->setEnabled(appSelected);
        m_actKillApp->setEnabled(appSelected);
        m_actToWildcard->setEnabled(appSelected);
        m_actEditApp->setEnabled(appSelected);
        m_actRemoveApp->setEnabled(appSelected);
        m_btAllowApp->setEnabled(appSelected);
        m_btBlockApp->setEnabled(appSelected);
        m_btRemoveApp->setEnabled(appSelected);
        m_appInfoRow->setVisible(appSelected);
    };

    refreshTableAppsChanged();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshTableAppsChanged);
}

void ProgramsWindow::showTableAppsHeaderMenu(const QPoint &pos)
{
    auto menu = ControlUtil::createMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto header = m_appListView->horizontalHeader();

    setupTableAppsHeaderMenuColumns(menu, header);

    menu->addSeparator();

    // Stretch last column
    {
        auto a = new QAction(tr("Stretch last column"), menu);
        a->setCheckable(true);
        a->setChecked(header->stretchLastSection());

        connect(a, &QAction::triggered, this, [&](bool checked) {
            m_appListView->horizontalHeader()->setStretchLastSection(checked);
        });

        menu->addAction(a);
    }

    menu->popup(header->mapToGlobal(pos));
}

void ProgramsWindow::setupTableAppsHeaderMenuColumns(QMenu *menu, QHeaderView *header)
{
    const auto switchColumnVisible = [&](bool checked) {
        const auto action = qobject_cast<QAction *>(sender());
        const int column = action->data().toInt();

        auto header = m_appListView->horizontalHeader();

        header->setSectionHidden(column, !checked);
    };

    const bool canHide = (header->hiddenSectionCount() < int(AppListColumn::Count) - 1);

    for (int i = 0; i < int(AppListColumn::Count); ++i) {
        const auto name = AppListModel::columnName(AppListColumn(i));

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

void ProgramsWindow::onSortStateClicked(int sortState)
{
    if (sortState == appListModel()->sortState()) {
        sortState = AppListModel::SortNone;
    }

    appListModel()->setSortState(AppListModel::SortState(sortState));

    iniUser()->setProgWindowSortState(sortState);
}

void ProgramsWindow::updateSortStateCounts()
{
    const auto refreshSortStateCounts = [&] {
        const AppStatesCount asc = appListModel()->appStatesCount();

        setToolButtonCount(m_btSortAllowed, asc.allowed);
        setToolButtonCount(m_btSortBlocked, asc.blocked);
        setToolButtonCount(m_btSortAlerted, asc.alerted);
    };

    refreshSortStateCounts();

    connect(appListModel(), &AppListModel::modelReset, this, refreshSortStateCounts);
    connect(appListModel(), &AppListModel::dataChanged, this, refreshSortStateCounts);
}

bool ProgramsWindow::editProgramByPath(const QString &appPath)
{
    if (checkAppEditFormOpened())
        return false;

    const auto appRow = appListModel()->appRowByPath(appPath);

    openAppEditForm(appRow);
    return true;
}

void ProgramsWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

void ProgramsWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    const QString appPath = urls.first().toLocalFile();
    if (appPath.isEmpty())
        return;

    if (editProgramByPath(appPath)) {
        event->acceptProposedAction();
    }
}

void ProgramsWindow::addNewProgram()
{
    openAppEditForm({});
}

void ProgramsWindow::addNewWildcard()
{
    AppRow appRow;
    appRow.isWildcard = true;

    openAppEditForm(appRow);
}

void ProgramsWindow::convertToWildcard()
{
    const AppRow appRow = appListCurrentRow();
    if (appRow.isNull())
        return;

    if (appRow.isWildcard)
        return;

    windowManager()->showConfirmBox(
            [=, this] {
                App app = appRow.app();
                app.isWildcard = true;

                ctrl()->updateApp(app);
            },
            tr("Are you sure to convert selected program to wildcard?"));
}

void ProgramsWindow::editSelectedPrograms()
{
    const auto appIdList = selectedAppIdList();
    if (appIdList.isEmpty())
        return;

    const auto appRow = appListCurrentRow();
    if (appRow.isNull())
        return;

    openAppEditForm(appRow, appIdList);
}

void ProgramsWindow::openAppEditForm(const AppRow &appRow, const QVector<qint64> &appIdList)
{
    if (!m_formAppEdit) {
        m_formAppEdit = new ProgramEditDialog(ctrl(), this);

        m_formAppEdit->setExcludeFromCapture(this->excludeFromCapture());
    }

    m_formAppEdit->initialize(appRow, appIdList);

    WidgetWindow::showWidget(m_formAppEdit);

    m_formAppEdit->centerTo(this);
}

bool ProgramsWindow::checkAppEditFormOpened() const
{
    if (m_formAppEdit && m_formAppEdit->isVisible()) {
        WidgetWindow::showWidget(m_formAppEdit);
        return true;
    }
    return false;
}

void ProgramsWindow::updateSelectedApps(bool blocked, bool killProcess)
{
    ctrl()->updateAppsBlocked(selectedAppIdList(), blocked, killProcess);
}

void ProgramsWindow::deleteSelectedApps()
{
    const auto appIdList = selectedAppIdList();
    const QStringList appNames = getAppListNames(appIdList);

    windowManager()->showConfirmBox([=, this] { ctrl()->deleteApps(appIdList); },
            tr("Are you sure to remove selected program(s)?")
                    // App names
                    + "\n\n" + appNames.join('\n'));
}

void ProgramsWindow::clearAlerts()
{
    windowManager()->showConfirmBox(
            [=, this] { ctrl()->clearAlerts(); }, tr("Are you sure to clear alerts?"));
}

int ProgramsWindow::appListCurrentIndex() const
{
    return m_appListView->currentRow();
}

AppRow ProgramsWindow::appListCurrentRow() const
{
    return appListModel()->appRowAt(appListCurrentIndex());
}

QString ProgramsWindow::appListCurrentPath() const
{
    const auto appRow = appListCurrentRow();
    return appRow.isNull() ? QString() : appRow.appPath;
}

QStringList ProgramsWindow::getAppListNames(const QVector<qint64> &appIdList, int maxCount) const
{
    QStringList list;

    const int n = appIdList.size();
    for (int i = 0; i < n;) {
        const qint64 appId = appIdList[i];
        const auto appRow = appListModel()->appRowById(appId);

        ++i;
        list.append(QString("%1) ").arg(i) + appRow.appName);

        if (i >= maxCount && i < n) {
            list.append("...");
            i = n - 1;
        }
    }

    return list;
}

QVector<qint64> ProgramsWindow::selectedAppIdList() const
{
    QVector<qint64> list;

    const auto rows = m_appListView->selectedRows();
    for (int row : rows) {
        const auto &appRow = appListModel()->appRowAt(row);
        list.append(appRow.appId);
    }

    return list;
}
