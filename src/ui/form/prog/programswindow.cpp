#include "programswindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCloseEvent>
#include <QDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "../../conf/appgroup.h"
#include "../../conf/firewallconf.h"
#include "../../conf/confmanager.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../log/model/applistmodel.h"
#include "../../util/app/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/osutil.h"
#include "../controls/checkspincombo.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "../controls/widebutton.h"
#include "programscontroller.h"

namespace {

#define APPS_HEADER_VERSION 2

const ValuesList appBlockInHourValues = {
    3, 1, 6, 12, 24, 24 * 7, 24 * 30
};

}

ProgramsWindow::ProgramsWindow(FortManager *fortManager,
                               QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ProgramsController(fortManager, this)),
    m_appListModel(ctrl()->appListModel())
{
    setupAppListModel();

    setupUi();
    setupController();
}

void ProgramsWindow::setupController()
{
    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramsWindow::onRetranslateUi);

    connect(fortManager(), &FortManager::afterSaveProgWindowState,
            this, &ProgramsWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreProgWindowState,
            this, &ProgramsWindow::onRestoreWindowState);

    emit ctrl()->retranslateUi();
}

void ProgramsWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();
        ctrl()->closeWindow();
    }
}

void ProgramsWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_Escape:  // Esc
        if (event->modifiers() == Qt::NoModifier) {
            ctrl()->closeWindow();
        }
        break;
    }
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
    m_btEdit->setText(tr("Edit"));
    m_actAllowApp->setText(tr("Allow"));
    m_actBlockApp->setText(tr("Block"));
    m_actAddApp->setText(tr("Add"));
    m_actEditApp->setText(tr("Edit"));
    m_actRemoveApp->setText(tr("Remove"));
    m_actPurgeApps->setText(tr("Purge All"));

    m_btAllowApp->setText(tr("Allow"));
    m_btBlockApp->setText(tr("Block"));

    m_labelEditPath->setText(tr("Program Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelEditName->setText(tr("Program Name:"));
    m_labelAppGroup->setText(tr("Application Group:"));
    m_cbUseGroupPerm->setText(tr("Use Application Group's Enabled State"));
    m_rbAllowApp->setText(tr("Allow"));
    m_rbBlockApp->setText(tr("Block"));
    m_cscBlockApp->checkBox()->setText(tr("Block In:"));
    retranslateAppBlockInHours();
    m_btEditOk->setText(tr("OK"));
    m_btEditCancel->setText(tr("Cancel"));

    m_formAppEdit->setWindowTitle(tr("Edit Program"));

    m_btLogOptions->setText(tr("Options"));
    m_cbLogBlocked->setText(tr("Show New Programs"));

    appListModel()->refresh();

    m_btAppCopyPath->setToolTip(tr("Copy Path"));
    m_btAppOpenFolder->setToolTip(tr("Open Folder"));

    this->setWindowTitle(tr("Programs"));
}

void ProgramsWindow::retranslateAppBlockInHours()
{
    const QStringList list = {
        tr("Custom"), tr("1 hour"), tr("6 hours"),
        tr("12 hours"), tr("Day"), tr("Week"), tr("Month")
    };

    m_cscBlockApp->setNames(list);
    m_cscBlockApp->spinBox()->setSuffix(tr(" hours"));
}

void ProgramsWindow::setupAppListModel()
{
    appListModel()->initialize();
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
    setupAppInfoVersion();
    layout->addWidget(m_appInfoRow);

    // Actions on app table's current changed
    setupTableAppsChanged();

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png",
                                             ":/images/application_cascade.png"));

    // Size
    this->resize(1024, 768);
    this->setMinimumSize(500, 400);
}

QLayout *ProgramsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actAllowApp = editMenu->addAction(QIcon(":/images/arrow_switch.png"), QString());
    m_actAllowApp->setShortcut(Qt::Key_A);

    m_actBlockApp = editMenu->addAction(QIcon(":/images/stop.png"), QString());
    m_actBlockApp->setShortcut(Qt::Key_B);

    editMenu->addSeparator();

    m_actAddApp = editMenu->addAction(QIcon(":/images/application_add.png"), QString());
    m_actAddApp->setShortcut(Qt::Key_Plus);

    m_actEditApp = editMenu->addAction(QIcon(":/images/application_edit.png"), QString());
    m_actEditApp->setShortcut(Qt::Key_Enter);

    m_actRemoveApp = editMenu->addAction(QIcon(":/images/application_delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    editMenu->addSeparator();

    m_actPurgeApps = editMenu->addAction(QIcon(":/images/bin_empty.png"), QString());

    connect(m_actAllowApp, &QAction::triggered, [&] {
        updateSelectedApps(false);
    });
    connect(m_actBlockApp, &QAction::triggered, [&] {
        updateSelectedApps(true);
    });
    connect(m_actAddApp, &QAction::triggered, [&] {
        updateAppEditForm(false);
    });
    connect(m_actEditApp, &QAction::triggered, [&] {
        updateAppEditForm(true);
    });
    connect(m_actRemoveApp, &QAction::triggered, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove selected program(s)?"))) {
            deleteSelectedApps();
        }
    });
    connect(m_actPurgeApps, &QAction::triggered, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove all non-existent programs?"))) {
            appListModel()->purgeApps();
        }
    });

    m_btEdit = new WideButton(QIcon(":/images/application_edit.png"));
    m_btEdit->setMenu(editMenu);

    // Allow/Block
    m_btAllowApp = ControlUtil::createLinkButton(":/images/arrow_switch.png");
    m_btBlockApp = ControlUtil::createLinkButton(":/images/stop.png");

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

void ProgramsWindow::setupAppEditForm()
{
    auto formLayout = new QFormLayout();

    // App Path
    auto pathLayout = new QHBoxLayout();

    m_editPath = new QLineEdit();

    m_btSelectFile = ControlUtil::createLinkButton(":/images/folder_explore.png");

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

    // Allow/Block
    auto allowLayout = new QHBoxLayout();
    allowLayout->setSpacing(20);

    m_rbAllowApp = new QRadioButton();
    m_rbAllowApp->setIcon(QIcon(":/images/arrow_switch.png"));
    m_rbAllowApp->setChecked(true);

    m_rbBlockApp = new QRadioButton();
    m_rbBlockApp->setIcon(QIcon(":/images/stop.png"));

    allowLayout->addWidget(m_rbAllowApp, 1, Qt::AlignRight);
    allowLayout->addWidget(m_rbBlockApp, 1, Qt::AlignLeft);

    // Block after N hours
    m_cscBlockApp = new CheckSpinCombo();
    m_cscBlockApp->spinBox()->setRange(1, 24 * 30 * 12);  // ~Year
    m_cscBlockApp->setValues(appBlockInHourValues);
    m_cscBlockApp->setNamesByValues();

    // OK/Cancel
    auto buttonsLayout = new QHBoxLayout();

    m_btEditOk = new QPushButton(QIcon(":/images/tick.png"), QString());
    m_btEditOk->setDefault(true);

    m_btEditCancel = new QPushButton(QIcon(":/images/cancel.png"), QString());

    buttonsLayout->addWidget(m_btEditOk, 1, Qt::AlignRight);
    buttonsLayout->addWidget(m_btEditCancel);

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(allowLayout);
    layout->addWidget(m_cscBlockApp);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    m_formAppEdit = new QDialog(this);
    m_formAppEdit->setWindowModality(Qt::WindowModal);
    m_formAppEdit->setSizeGripEnabled(true);
    m_formAppEdit->setLayout(layout);
    m_formAppEdit->setMinimumWidth(500);

    connect(m_btSelectFile, &QAbstractButton::clicked, [&] {
        const auto filePath = ControlUtil::getOpenFileName(
                    m_labelEditPath->text(),
                    tr("Programs (*.exe);;All files (*.*)"));

        if (!filePath.isEmpty()) {
            m_editPath->setText(filePath);
        }
    });

    connect(m_rbAllowApp, &QRadioButton::toggled, m_cscBlockApp, &CheckSpinCombo::setEnabled);

    connect(m_btEditOk, &QAbstractButton::clicked, [&] {
        const QString appPath = m_editPath->text();
        if (appPath.isEmpty())
            return;

        const QString appName = m_editName->text();
        const int groupIndex = m_comboAppGroup->currentIndex();
        const bool useGroupPerm = m_cbUseGroupPerm->isChecked();
        const bool blocked = m_rbBlockApp->isChecked();

        QDateTime endTime;
        if (!blocked && m_cscBlockApp->checkBox()->isChecked()) {
            const int hours = m_cscBlockApp->spinBox()->value();

            endTime = QDateTime::currentDateTime()
                    .addSecs(hours * 60 * 60);
        }

        const auto appRow = appListModel()->appRowAt(groupIndex);

        const bool appNameEdited = (appName != appRow.appName);
        const bool appEdited = (appPath != appRow.appPath
                || groupIndex != appRow.groupIndex
                || useGroupPerm != appRow.useGroupPerm
                || blocked != appRow.blocked
                || endTime != appRow.endTime);

        if (!(appNameEdited || appEdited)
                || (m_formAppId != 0
                    ? appListModel()->updateApp(m_formAppId, appPath, appName, endTime,
                                                groupIndex, useGroupPerm, blocked, appEdited)
                    : appListModel()->addApp(appPath, appName, endTime, groupIndex,
                                             useGroupPerm, blocked, appEdited))) {
            m_formAppEdit->close();
        }
    });
    connect(m_btEditCancel, &QAbstractButton::clicked, m_formAppEdit, &QWidget::close);
}

void ProgramsWindow::setupComboAppGroups()
{
    m_comboAppGroup = new QComboBox();

    const auto refreshComboAppGroups = [&](bool onlyFlags = false) {
        if (onlyFlags) return;

        m_comboAppGroup->clear();
        m_comboAppGroup->addItems(appListModel()->appGroupNames());
        m_comboAppGroup->setCurrentIndex(0);
    };

    refreshComboAppGroups();

    connect(confManager(), &ConfManager::confSaved, this, refreshComboAppGroups);
}

void ProgramsWindow::setupLogOptions()
{
    setupLogBlocked();

    // Menu
    const QList<QWidget *> menuWidgets = {
        m_cbLogBlocked
    };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btLogOptions = new WideButton(QIcon(":/images/application_key.png"));
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
    m_appListView->setIconSize(QSize(24, 24));
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
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);

    header->resizeSection(0, 540);
    header->resizeSection(2, 100);

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(4, Qt::DescendingOrder);
}

void ProgramsWindow::setupAppInfoRow()
{
    auto layout = new QHBoxLayout();
    layout->setMargin(0);

    m_btAppCopyPath = ControlUtil::createLinkButton(":/images/page_copy.png");
    m_btAppOpenFolder = ControlUtil::createLinkButton(":/images/folder_go.png");

    m_lineAppPath = ControlUtil::createLineLabel();

    m_labelAppProductName = ControlUtil::createLabel();
    m_labelAppProductName->setFont(ControlUtil::fontDemiBold());

    m_labelAppCompanyName = ControlUtil::createLabel();

    connect(m_btAppCopyPath, &QAbstractButton::clicked, [&] {
        GuiUtil::setClipboardData(appListCurrentPath());
    });
    connect(m_btAppOpenFolder, &QAbstractButton::clicked, [&] {
        OsUtil::openFolder(appListCurrentPath());
    });

    layout->addWidget(m_btAppCopyPath);
    layout->addWidget(m_btAppOpenFolder);
    layout->addWidget(m_lineAppPath, 1);
    layout->addWidget(m_labelAppProductName);
    layout->addWidget(m_labelAppCompanyName);

    m_appInfoRow = new QWidget();
    m_appInfoRow->setLayout(layout);
}

void ProgramsWindow::setupAppInfoVersion()
{
    const auto refreshAppInfoVersion = [&] {
        const auto appPath = appListCurrentPath();
        const auto appInfo = appInfoCache()->appInfo(appPath);

        m_lineAppPath->setText(appPath);
        m_lineAppPath->setToolTip(appPath);

        m_labelAppProductName->setVisible(!appInfo.productName.isEmpty());
        m_labelAppProductName->setText(appInfo.productName + " v" + appInfo.productVersion);

        m_labelAppCompanyName->setVisible(!appInfo.companyName.isEmpty());
        m_labelAppCompanyName->setText(appInfo.companyName);
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
        m_actEditApp->setEnabled(appSelected);
        m_actRemoveApp->setEnabled(appSelected);
        m_btAllowApp->setEnabled(appSelected);
        m_btBlockApp->setEnabled(appSelected);
        m_appInfoRow->setVisible(appSelected);
    };

    refreshTableAppsChanged();

    connect(m_appListView, &TableView::currentIndexChanged, this, refreshTableAppsChanged);
}

void ProgramsWindow::updateAppEditForm(bool editCurrentApp)
{
    AppRow appRow;
    if (editCurrentApp) {
        const auto appIndex = appListCurrentIndex();
        if (appIndex < 0) return;

        appRow = appListModel()->appRowAt(appIndex);
        m_formAppId = appRow.appId;
    } else {
        m_formAppId = 0;
    }

    m_editPath->setText(appRow.appPath);
    m_editPath->setReadOnly(editCurrentApp);
    m_editPath->setClearButtonEnabled(!editCurrentApp);
    m_editPath->selectAll();
    m_editPath->setFocus();
    m_btSelectFile->setEnabled(!editCurrentApp);
    m_editName->setText(appRow.appName);
    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);
    m_cbUseGroupPerm->setChecked(appRow.useGroupPerm);
    m_rbAllowApp->setChecked(!appRow.blocked);
    m_rbBlockApp->setChecked(appRow.blocked);
    m_cscBlockApp->setEnabled(!appRow.blocked);
    m_cscBlockApp->checkBox()->setChecked(false);

    m_formAppEdit->show();
}

void ProgramsWindow::updateApp(int row, bool blocked)
{
    const auto appRow = appListModel()->appRowAt(row);
    appListModel()->updateApp(appRow.appId, appRow.appPath, appRow.appName,
                              QDateTime(), appRow.groupIndex,
                              appRow.useGroupPerm, blocked);
}

void ProgramsWindow::deleteApp(int row)
{
    const auto appRow = appListModel()->appRowAt(row);
    appListModel()->deleteApp(appRow.appId, appRow.appPath, row);
}

void ProgramsWindow::updateSelectedApps(bool blocked)
{
    const auto rows = m_appListView->selectedRows();
    for (int i = rows.size(); --i >= 0; ) {
        updateApp(rows.at(i), blocked);
    }
}

void ProgramsWindow::deleteSelectedApps()
{
    const auto rows = m_appListView->selectedRows();
    for (int i = rows.size(); --i >= 0; ) {
        deleteApp(rows.at(i));
    }
}

int ProgramsWindow::appListCurrentIndex() const
{
    return m_appListView->currentIndex().row();
}

QString ProgramsWindow::appListCurrentPath() const
{
    const auto appRow = appListModel()->appRowAt(appListCurrentIndex());
    return appRow.appPath;
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

AppInfoCache *ProgramsWindow::appInfoCache() const
{
    return appListModel()->appInfoCache();
}
