#include "programswindow.h"

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
#include "../../model/applistmodel.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../../util/window/widgetwindowstatewatcher.h"
#include "../controls/appinforow.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "programeditdialog.h"
#include "programscontroller.h"

namespace {

#define APPS_HEADER_VERSION 3

}

ProgramsWindow::ProgramsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ProgramsController(fortManager, this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupAppEditForm();
    setupController();
    setupStateWatcher();
}

FortManager *ProgramsWindow::fortManager() const
{
    return ctrl()->fortManager();
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

AppListModel *ProgramsWindow::appListModel() const
{
    return fortManager()->appListModel();
}

AppInfoCache *ProgramsWindow::appInfoCache() const
{
    return appListModel()->appInfoCache();
}

void ProgramsWindow::saveWindowState()
{
    settings()->setProgWindowGeometry(m_stateWatcher->geometry());
    settings()->setProgWindowMaximized(m_stateWatcher->maximized());

    auto header = m_appListView->horizontalHeader();
    settings()->setProgAppsHeader(header->saveState());
    settings()->setProgAppsHeaderVersion(APPS_HEADER_VERSION);
}

void ProgramsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), settings()->progWindowGeometry(),
            settings()->progWindowMaximized());

    if (settings()->progAppsHeaderVersion() == APPS_HEADER_VERSION) {
        auto header = m_appListView->horizontalHeader();
        header->restoreState(settings()->progAppsHeader());
    }
}

void ProgramsWindow::setupController()
{
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
    m_actAddApp->setText(tr("Add"));
    m_actEditApp->setText(tr("Edit"));
    m_actRemoveApp->setText(tr("Remove"));
    m_actPurgeApps->setText(tr("Purge All"));

    m_btAllowApp->setText(tr("Allow"));
    m_btBlockApp->setText(tr("Block"));
    m_btRemoveApp->setText(tr("Remove"));

    m_btLogOptions->setText(tr("Options"));
    m_cbLogBlocked->setText(tr("Collect New Blocked Programs"));

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

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png", ":/icons/window.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ProgramsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actAllowApp = editMenu->addAction(IconCache::icon(":/icons/sign-check.png"), QString());
    m_actAllowApp->setShortcut(Qt::Key_A);

    m_actBlockApp = editMenu->addAction(IconCache::icon(":/icons/sign-ban.png"), QString());
    m_actBlockApp->setShortcut(Qt::Key_B);

    editMenu->addSeparator();

    m_actAddApp = editMenu->addAction(IconCache::icon(":/icons/sign-add.png"), QString());
    m_actAddApp->setShortcut(Qt::Key_Plus);

    m_actEditApp = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditApp->setShortcut(Qt::Key_Enter);

    m_actRemoveApp = editMenu->addAction(IconCache::icon(":/icons/sign-delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    editMenu->addSeparator();

    m_actPurgeApps = editMenu->addAction(IconCache::icon(":/icons/trashcan-full.png"), QString());

    connect(m_actAllowApp, &QAction::triggered, this, [&] { updateSelectedApps(false); });
    connect(m_actBlockApp, &QAction::triggered, this, [&] { updateSelectedApps(true); });
    connect(m_actAddApp, &QAction::triggered, this, &ProgramsWindow::addNewProgram);
    connect(m_actEditApp, &QAction::triggered, this, &ProgramsWindow::editSelectedPrograms);
    connect(m_actRemoveApp, &QAction::triggered, this, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove selected program(s)?"))) {
            deleteSelectedApps();
        }
    });
    connect(m_actPurgeApps, &QAction::triggered, this, [&] {
        if (fortManager()->showQuestionBox(
                    tr("Are you sure to remove all non-existent programs?"))) {
            appListModel()->purgeApps();
        }
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Toolbar buttons
    m_btAllowApp = ControlUtil::createLinkButton(":/icons/sign-check.png");
    m_btBlockApp = ControlUtil::createLinkButton(":/icons/sign-ban.png");
    m_btRemoveApp = ControlUtil::createLinkButton(":/icons/sign-delete.png");

    connect(m_btAllowApp, &QAbstractButton::clicked, m_actAllowApp, &QAction::trigger);
    connect(m_btBlockApp, &QAbstractButton::clicked, m_actBlockApp, &QAction::trigger);
    connect(m_btRemoveApp, &QAbstractButton::clicked, m_actRemoveApp, &QAction::trigger);

    // Log Options
    setupLogOptions();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btAllowApp);
    layout->addWidget(m_btBlockApp);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btRemoveApp);
    layout->addStretch();
    layout->addWidget(m_btLogOptions);

    return layout;
}

void ProgramsWindow::setupLogOptions()
{
    setupLogBlocked();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbLogBlocked };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btLogOptions = ControlUtil::createButton(":/icons/wrench.png");
    m_btLogOptions->setMenu(menu);
}

void ProgramsWindow::setupLogBlocked()
{
    m_cbLogBlocked = ControlUtil::createCheckBox(conf()->logBlocked(), [&](bool checked) {
        if (conf()->logBlocked() == checked)
            return;

        conf()->setLogBlocked(checked);

        confManager()->saveFlags(true);
    });

    m_cbLogBlocked->setFont(ControlUtil::fontDemiBold());
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
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->setSectionResizeMode(4, QHeaderView::Stretch);

    header->resizeSection(0, 600);
    header->resizeSection(1, 120);
    header->resizeSection(2, 100);
    header->resizeSection(3, 30);

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(4, Qt::DescendingOrder);
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

void ProgramsWindow::setupAppEditForm()
{
    m_formAppEdit = new ProgramEditDialog(ctrl(), this);
}

bool ProgramsWindow::editProgramByPath(const QString &appPath)
{
    if (m_formAppEdit->isVisible()) {
        m_formAppEdit->activate();
        return false;
    }

    const auto appRow = appListModel()->appRowByPath(appPath);

    openAppEditForm(appRow);
    return true;
}

void ProgramsWindow::addNewProgram()
{
    openAppEditForm({});
}

void ProgramsWindow::editSelectedPrograms()
{
    const auto rows = m_appListView->selectedRows();
    if (rows.isEmpty())
        return;

    bool isFirstAppRow = true;
    AppRow firstAppRow;
    QVector<qint64> appIdList;

    for (int row : rows) {
        const auto appRow = appListModel()->appRowAt(row);
        if (isFirstAppRow) {
            isFirstAppRow = false;
            firstAppRow = appRow;
        }
        appIdList.append(appRow.appId);
    }

    openAppEditForm(firstAppRow, appIdList);
}

void ProgramsWindow::openAppEditForm(const AppRow &appRow, const QVector<qint64> &appIdList)
{
    m_formAppEdit->initialize(appRow, appIdList);

    m_formAppEdit->show();
    m_formAppEdit->activate();
}

void ProgramsWindow::updateApp(int row, bool blocked)
{
    const auto appRow = appListModel()->appRowAt(row);
    appListModel()->updateApp(appRow.appId, appRow.appPath, appRow.appName, QDateTime(),
            appRow.groupIndex, appRow.useGroupPerm, blocked);
}

void ProgramsWindow::deleteApp(int row)
{
    const auto appRow = appListModel()->appRowAt(row);
    appListModel()->deleteApp(appRow.appId, appRow.appPath, row);
}

void ProgramsWindow::updateSelectedApps(bool blocked)
{
    const auto rows = m_appListView->selectedRows();
    for (int i = rows.size(); --i >= 0;) {
        updateApp(rows.at(i), blocked);
    }
}

void ProgramsWindow::deleteSelectedApps()
{
    const auto rows = m_appListView->selectedRows();
    for (int i = rows.size(); --i >= 0;) {
        deleteApp(rows.at(i));
    }
}

int ProgramsWindow::appListCurrentIndex() const
{
    return m_appListView->currentRow();
}

QString ProgramsWindow::appListCurrentPath() const
{
    const auto appRow = appListModel()->appRowAt(appListCurrentIndex());
    return appRow.appPath;
}
