#include "programswindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "../../conf/appgroup.h"
#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/applistmodel.h"
#include "../../util/app/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../controls/appinforow.h"
#include "../controls/checkspincombo.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "programscontroller.h"

namespace {

#define APPS_HEADER_VERSION 3

const ValuesList appBlockInHourValues = { 3, 1, 6, 12, 24, 24 * 7, 24 * 30 };

}

ProgramsWindow::ProgramsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent), m_ctrl(new ProgramsController(fortManager, this))
{
    setupUi();
    setupController();
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

void ProgramsWindow::setupController()
{
    connect(this, &ProgramsWindow::aboutToClose, fortManager(), &FortManager::closeProgramsWindow);

    connect(fortManager(), &FortManager::afterSaveProgWindowState, this,
            &ProgramsWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreProgWindowState, this,
            &ProgramsWindow::onRestoreWindowState);

    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramsWindow::onRetranslateUi);

    emit ctrl()->retranslateUi();
}

void ProgramsWindow::onSaveWindowState()
{
    auto header = m_appListView->horizontalHeader();
    settings()->setProgAppsHeader(header->saveState());
    settings()->setProgAppsHeaderVersion(APPS_HEADER_VERSION);
}

void ProgramsWindow::onRestoreWindowState()
{
    if (settings()->progAppsHeaderVersion() != APPS_HEADER_VERSION)
        return;

    auto header = m_appListView->horizontalHeader();
    header->restoreState(settings()->progAppsHeader());
}

void ProgramsWindow::onRetranslateUi()
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

    m_formAppEdit->unsetLocale();
    m_formAppEdit->setWindowTitle(tr("Edit Program"));

    m_labelEditPath->setText(tr("Program Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelEditName->setText(tr("Program Name:"));
    m_labelAppGroup->setText(tr("Application Group:"));
    m_cbUseGroupPerm->setText(tr("Use Application Group's Enabled State"));
    m_rbAllowApp->setText(tr("Allow"));
    m_rbBlockApp->setText(tr("Block"));
    m_cscBlockAppIn->checkBox()->setText(tr("Block In:"));
    retranslateAppBlockInHours();
    m_cbBlockAppAt->setText(tr("Block At:"));
    m_dteBlockAppAt->unsetLocale();
    m_cbBlockAppNone->setText(tr("Forever"));

    m_btEditOk->setText(tr("OK"));
    m_btEditCancel->setText(tr("Cancel"));

    m_btLogOptions->setText(tr("Options"));
    m_cbLogBlocked->setText(tr("Collect New Blocked Programs"));

    appListModel()->refresh();

    m_appInfoRow->retranslateUi();

    this->setWindowTitle(tr("Programs"));
}

void ProgramsWindow::retranslateAppBlockInHours()
{
    const QStringList list = { tr("Custom"), tr("1 hour"), tr("6 hours"), tr("12 hours"), tr("Day"),
        tr("Week"), tr("Month") };

    m_cscBlockAppIn->setNames(list);
    m_cscBlockAppIn->spinBox()->setSuffix(tr(" hour(s)"));
}

void ProgramsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // App Add/Edit Form
    setupAppEditForm();

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

void ProgramsWindow::setupAppEditForm()
{
    auto formLayout = setupAppEditFormAppLayout();

    // Allow/Block
    auto allowLayout = setupAppEditFormAllowLayout();

    // Block at specified date & time
    auto blockAtLayout = setupCheckDateTimeEdit();

    // Eclusive End Time CheckBoxes Group
    setupAllowEclusiveGroup();

    // OK/Cancel
    auto buttonsLayout = new QHBoxLayout();

    m_btEditOk = new QPushButton();
    m_btEditOk->setDefault(true);

    m_btEditCancel = new QPushButton();

    buttonsLayout->addWidget(m_btEditOk, 1, Qt::AlignRight);
    buttonsLayout->addWidget(m_btEditCancel);

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(allowLayout);
    layout->addWidget(m_cscBlockAppIn);
    layout->addLayout(blockAtLayout);
    layout->addWidget(m_cbBlockAppNone);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    m_formAppEdit = new QDialog(this);
    m_formAppEdit->setWindowModality(Qt::WindowModal);
    m_formAppEdit->setSizeGripEnabled(true);
    m_formAppEdit->setLayout(layout);
    m_formAppEdit->setMinimumWidth(500);

    connect(m_btSelectFile, &QAbstractButton::clicked, this, [&] {
        const auto filePath = ControlUtil::getOpenFileName(
                m_labelEditPath->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (!filePath.isEmpty()) {
            m_editPath->setText(filePath);
        }
    });

    connect(m_rbAllowApp, &QRadioButton::toggled, this, [&](bool checked) {
        m_cbBlockAppNone->setEnabled(checked);
        m_cscBlockAppIn->setEnabled(checked);
        m_cbBlockAppAt->setEnabled(checked);
        m_dteBlockAppAt->setEnabled(checked);
    });

    connect(m_btEditOk, &QAbstractButton::clicked, this, [&] {
        if (saveAppEditForm()) {
            m_formAppEdit->close();
        }
    });
    connect(m_btEditCancel, &QAbstractButton::clicked, m_formAppEdit, &QWidget::close);
}

QLayout *ProgramsWindow::setupAppEditFormAppLayout()
{
    auto formLayout = new QFormLayout();

    // App Path
    auto pathLayout = new QHBoxLayout();

    m_editPath = new QLineEdit();

    m_btSelectFile = ControlUtil::createLinkButton(":/icons/folder-open.png");

    pathLayout->addWidget(m_editPath);
    pathLayout->addWidget(m_btSelectFile);

    formLayout->addRow("Program Path:", pathLayout);
    m_labelEditPath = qobject_cast<QLabel *>(formLayout->labelForField(pathLayout));

    // App Name
    m_editName = new QLineEdit();

    formLayout->addRow("Program Name:", m_editName);
    m_labelEditName = qobject_cast<QLabel *>(formLayout->labelForField(m_editName));

    // App Group
    setupComboAppGroups();

    formLayout->addRow("Application Group:", m_comboAppGroup);
    m_labelAppGroup = qobject_cast<QLabel *>(formLayout->labelForField(m_comboAppGroup));

    // Use Group Perm.
    m_cbUseGroupPerm = new QCheckBox();

    formLayout->addRow(QString(), m_cbUseGroupPerm);

    return formLayout;
}

void ProgramsWindow::setupComboAppGroups()
{
    m_comboAppGroup = new QComboBox();

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        m_comboAppGroup->clear();
        m_comboAppGroup->addItems(appListModel()->appGroupNames());
        m_comboAppGroup->setCurrentIndex(0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confSaved, this, refreshComboAppGroups);
}

QLayout *ProgramsWindow::setupAppEditFormAllowLayout()
{
    auto allowLayout = new QHBoxLayout();
    allowLayout->setSpacing(20);

    m_rbAllowApp = new QRadioButton();
    m_rbAllowApp->setIcon(IconCache::icon(":/icons/sign-check.png"));
    m_rbAllowApp->setChecked(true);

    m_rbBlockApp = new QRadioButton();
    m_rbBlockApp->setIcon(IconCache::icon(":/icons/sign-ban.png"));

    allowLayout->addWidget(m_rbAllowApp, 1, Qt::AlignRight);
    allowLayout->addWidget(m_rbBlockApp, 1, Qt::AlignLeft);

    // Block after N hours
    m_cscBlockAppIn = new CheckSpinCombo();
    m_cscBlockAppIn->spinBox()->setRange(1, 24 * 30 * 12); // ~Year
    m_cscBlockAppIn->setValues(appBlockInHourValues);
    m_cscBlockAppIn->setNamesByValues();

    // Allow Forever
    m_cbBlockAppNone = new QCheckBox();

    return allowLayout;
}

QLayout *ProgramsWindow::setupCheckDateTimeEdit()
{
    m_cbBlockAppAt = new QCheckBox();

    m_dteBlockAppAt = new QDateTimeEdit();
    m_dteBlockAppAt->setCalendarPopup(true);

    return ControlUtil::createRowLayout(m_cbBlockAppAt, m_dteBlockAppAt);
}

void ProgramsWindow::setupAllowEclusiveGroup()
{
    auto group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_cscBlockAppIn->checkBox());
    group->addButton(m_cbBlockAppAt);
    group->addButton(m_cbBlockAppNone);
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
    connect(m_actAddApp, &QAction::triggered, this, [&] { updateAppEditForm(false); });
    connect(m_actEditApp, &QAction::triggered, this, [&] { updateAppEditForm(true); });
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

    // Allow/Block
    m_btAllowApp = ControlUtil::createLinkButton(":/icons/sign-check.png");
    m_btBlockApp = ControlUtil::createLinkButton(":/icons/sign-ban.png");

    connect(m_btAllowApp, &QAbstractButton::clicked, m_actAllowApp, &QAction::trigger);
    connect(m_btBlockApp, &QAbstractButton::clicked, m_actBlockApp, &QAction::trigger);

    // Log Options
    setupLogOptions();

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btAllowApp);
    layout->addWidget(m_btBlockApp);
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

        fortManager()->applyConfImmediateFlags();
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
        m_appInfoRow->setVisible(appSelected);
    };

    refreshTableAppsChanged();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshTableAppsChanged);
}

bool ProgramsWindow::openAppEditFormByPath(const QString &appPath)
{
    if (m_formAppEdit->isVisible()) {
        activateAppEditForm();
        return false;
    }

    const auto appRow = appListModel()->appRowByPath(appPath);

    openAppEditFormByRow(appRow, false, true);
    return true;
}

void ProgramsWindow::updateAppEditForm(bool editCurrentApp)
{
    bool isSingleSelection = true;

    AppRow appRow;
    if (editCurrentApp) {
        const auto rows = m_appListView->selectedRows();
        if (rows.isEmpty())
            return;

        isSingleSelection = (rows.size() == 1);

        const auto appIndex = appListCurrentIndex();
        appRow = appListModel()->appRowAt(appIndex);
    }

    openAppEditFormByRow(appRow, editCurrentApp, isSingleSelection);
}

void ProgramsWindow::openAppEditFormByRow(
        const AppRow &appRow, bool editCurrentApp, bool isSingleSelection)
{
    m_formAppIsNew = !editCurrentApp;

    m_editPath->setText(isSingleSelection ? appRow.appPath : "*");
    m_editPath->setReadOnly(editCurrentApp);
    m_editPath->setClearButtonEnabled(!editCurrentApp);
    m_editPath->setEnabled(isSingleSelection);
    m_btSelectFile->setEnabled(!editCurrentApp);
    m_editName->setText(isSingleSelection ? appRow.appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);
    m_cbUseGroupPerm->setChecked(appRow.useGroupPerm);
    m_rbAllowApp->setChecked(!appRow.blocked);
    m_rbBlockApp->setChecked(appRow.blocked);
    m_cscBlockAppIn->checkBox()->setChecked(false);
    m_cscBlockAppIn->spinBox()->setValue(1);
    m_cbBlockAppAt->setChecked(!appRow.endTime.isNull());
    m_dteBlockAppAt->setDateTime(appRow.endTime);
    m_dteBlockAppAt->setMinimumDateTime(QDateTime::currentDateTime());
    m_cbBlockAppNone->setChecked(appRow.endTime.isNull());

    m_formAppEdit->show();

    activateAppEditForm();
}

void ProgramsWindow::activateAppEditForm()
{
    m_formAppEdit->activateWindow();
    m_editPath->selectAll();
    m_editPath->setFocus();
}

bool ProgramsWindow::saveAppEditForm()
{
    QString appPath = m_editPath->text();
    if (appPath.isEmpty())
        return false;

    QString appName = m_editName->text();
    const int groupIndex = m_comboAppGroup->currentIndex();
    const bool useGroupPerm = m_cbUseGroupPerm->isChecked();
    const bool blocked = m_rbBlockApp->isChecked();

    QDateTime endTime;
    if (!blocked) {
        if (m_cscBlockAppIn->checkBox()->isChecked()) {
            const int hours = m_cscBlockAppIn->spinBox()->value();

            endTime = QDateTime::currentDateTime().addSecs(hours * 60 * 60);
        } else if (m_cbBlockAppAt->isChecked()) {
            endTime = m_dteBlockAppAt->dateTime();
        }
    }

    // Add new app
    if (m_formAppIsNew) {
        return appListModel()->addApp(appPath, appName, endTime, groupIndex, useGroupPerm, blocked);
    }

    // Edit selected apps
    return saveAppEditFormMulti(appPath, appName, endTime, groupIndex, useGroupPerm, blocked);
}

bool ProgramsWindow::saveAppEditFormMulti(const QString &appPath, const QString &appName,
        const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool blocked)
{
    const auto rows = m_appListView->selectedRows();
    const bool isSingleSelection = (rows.size() == 1);

    bool updateDriver = true;

    if (isSingleSelection) {
        const int appIndex = appListCurrentIndex();
        const auto appRow = appListModel()->appRowAt(appIndex);

        const bool appNameEdited = (appName != appRow.appName);
        const bool appEdited = (appPath != appRow.appPath || groupIndex != appRow.groupIndex
                || useGroupPerm != appRow.useGroupPerm || blocked != appRow.blocked
                || endTime != appRow.endTime);

        if (!appEdited) {
            if (appNameEdited) {
                return appListModel()->updateAppName(appRow.appId, appName);
            }
            return true;
        }

        updateDriver = appEdited;
    }

    for (int row : rows) {
        const auto appRow = appListModel()->appRowAt(row);

        const auto rowAppPath = isSingleSelection ? appPath : appRow.appPath;
        const auto rowAppName = isSingleSelection ? appName : appRow.appName;

        if (!appListModel()->updateApp(appRow.appId, rowAppPath, rowAppName, endTime, groupIndex,
                    useGroupPerm, blocked, updateDriver))
            return false;
    }

    return true;
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
