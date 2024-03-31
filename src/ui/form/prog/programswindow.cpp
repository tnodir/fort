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

constexpr int APPS_HEADER_VERSION = 11;

}

ProgramsWindow::ProgramsWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ProgramsController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
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
    iniUser()->setProgWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setProgWindowMaximized(m_stateWatcher->maximized());

    auto header = m_appListView->horizontalHeader();
    iniUser()->setProgAppsHeader(header->saveState());
    iniUser()->setProgAppsHeaderVersion(APPS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ProgramsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), iniUser()->progWindowGeometry(),
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

void ProgramsWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
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
    m_actEditApp->setText(tr("Edit"));
    m_actRemoveApp->setText(tr("Remove"));
    m_actReviewAlerts->setText(tr("Review Alerts"));
    m_actPurgeApps->setText(tr("Purge Obsolete"));
    m_actFindApps->setText(tr("Find"));

    m_btAllowApp->setText(tr("Allow"));
    m_btBlockApp->setText(tr("Block"));
    m_btRemoveApp->setText(tr("Remove"));
    m_editSearch->setPlaceholderText(tr("Search"));

    m_btGroups->setText(tr("Groups"));
    m_btServices->setText(tr("Services"));

    appListModel()->refresh();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Programs"));
}

void ProgramsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableApps();
    setupTableAppsHeader();
    layout->addWidget(m_appListView, 1);

    // App Info Row
    setupAppInfoRow();
    layout->addWidget(m_appInfoRow);

    // Actions on apps table's current changed
    setupTableAppsChanged();

    this->setLayout(layout);

    // Accept File Drops
    setAcceptDrops(true);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/application.png"));

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

    // Groups button
    m_btGroups = ControlUtil::createFlatToolButton(":/icons/application_double.png");

    connect(m_btGroups, &QAbstractButton::clicked, windowManager(),
            &WindowManager::showAppGroupsWindow);

    // Services button
    m_btServices = ControlUtil::createFlatToolButton(":/icons/windows-48.png");
    m_btServices->setEnabled(settings()->hasMasterAdmin());

    connect(m_btServices, &QAbstractButton::clicked, windowManager(),
            &WindowManager::showServicesWindow);

    // Menu button
    m_btMenu = windowManager()->createMenuButton();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btAllowApp);
    layout->addWidget(m_btBlockApp);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btRemoveApp);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_editSearch);
    layout->addStretch();
    layout->addWidget(m_btGroups);
    layout->addWidget(m_btServices);
    layout->addWidget(ControlUtil::createVSeparator());
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

    m_actEditApp = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditApp->setShortcut(Qt::Key_Enter);

    m_actRemoveApp = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    editMenu->addSeparator();

    m_actReviewAlerts = editMenu->addAction(IconCache::icon(":/icons/error.png"), QString());
    m_actPurgeApps = editMenu->addAction(IconCache::icon(":/icons/recycle.png"), QString());

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
    connect(m_actEditApp, &QAction::triggered, this, &ProgramsWindow::editSelectedPrograms);
    connect(m_actRemoveApp, &QAction::triggered, this, &ProgramsWindow::deleteSelectedApps);
    connect(m_actReviewAlerts, &QAction::triggered, this,
            [&] { windowManager()->showProgramAlertWindow(); });
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

void ProgramsWindow::setupTableApps()
{
    m_appListView = new TableView();
    m_appListView->setAlternatingRowColors(true);
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

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->setSectionResizeMode(4, QHeaderView::Interactive);
    header->setSectionResizeMode(5, QHeaderView::Interactive);
    header->setSectionResizeMode(6, QHeaderView::Interactive);
    header->setSectionResizeMode(7, QHeaderView::Stretch);

    header->resizeSection(0, 300);
    header->resizeSection(1, 30);
    header->resizeSection(2, 30);
    header->resizeSection(3, 30);
    header->resizeSection(4, 100);
    header->resizeSection(5, 100);
    header->resizeSection(6, 270);

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(7, Qt::DescendingOrder);
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

void ProgramsWindow::editSelectedPrograms()
{
    const QVector<qint64> appIdList = selectedAppIdList();
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
