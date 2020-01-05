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
#include "programscontroller.h"

namespace {

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
    setupController();

    setupUi();

    emit ctrl()->retranslateUi();
}

void ProgramsWindow::setupController()
{
    connect(ctrl(), &ProgramsController::retranslateUi, this, &ProgramsWindow::onRetranslateUi);

    connect(fortManager(), &FortManager::afterSaveProgWindowState,
            this, &ProgramsWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreProgWindowState,
            this, &ProgramsWindow::onRestoreWindowState);
}

void ProgramsWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();
        ctrl()->closeWindow();
    }
}

void ProgramsWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape
            && event->modifiers() == Qt::NoModifier) {
        ctrl()->closeWindow();
    }
}

void ProgramsWindow::onSaveWindowState()
{
    auto header = m_appListView->horizontalHeader();
    settings()->setProgAppsHeader(header->saveState());
}

void ProgramsWindow::onRestoreWindowState()
{
    auto header = m_appListView->horizontalHeader();
    header->restoreState(settings()->progAppsHeader());
}

void ProgramsWindow::onRetranslateUi()
{
    m_btAddApp->setText(tr("Add"));
    m_btEditApp->setText(tr("Edit"));
    m_btDeleteApp->setText(tr("Delete"));

    m_btAllowApp->setText(tr("Allow"));
    m_btBlockApp->setText(tr("Block"));

    m_labelEditPath->setText(tr("Program Path:"));
    m_btSelectFile->setToolTip(tr("Select File"));
    m_labelAppGroup->setText(tr("Application Group:"));
    m_rbAllowApp->setText(tr("Allow"));
    m_rbBlockApp->setText(tr("Block"));
    m_cscBlockApp->checkBox()->setText(tr("Block In"));
    retranslateAppBlockInHours();
    m_btEditOk->setText(tr("OK"));
    m_btEditCancel->setText(tr("Cancel"));

    m_cbLogBlocked->setText(tr("Alert about Unknown Programs"));

    appListModel()->refresh();

    m_btAppCopyPath->setToolTip(tr("Copy Path"));
    m_btAppOpenFolder->setToolTip(tr("Open Folder"));
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

    // Title
    this->setWindowTitle(tr("Programs"));

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Size
    this->resize(1024, 768);
    this->setMinimumSize(500, 400);
}

QLayout *ProgramsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_btAddApp = ControlUtil::createLinkButton(":/images/application_add.png");
    m_btEditApp = ControlUtil::createLinkButton(":/images/application_edit.png");
    m_btDeleteApp = ControlUtil::createLinkButton(":/images/application_delete.png");

    m_btAllowApp = ControlUtil::createLinkButton(":/images/arrow_switch.png");
    m_btBlockApp = ControlUtil::createLinkButton(":/images/stop.png");

    connect(m_btAddApp, &QAbstractButton::clicked, [&] {
        updateAppEditForm(false);
    });
    connect(m_btEditApp, &QAbstractButton::clicked, [&] {
        updateAppEditForm(true);
    });
    connect(m_btDeleteApp, &QAbstractButton::clicked, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove the selected program?"))) {
            deleteCurrentApp();
        }
    });

    connect(m_btAllowApp, &QAbstractButton::clicked, [&] {
        updateCurrentApp(false);
    });
    connect(m_btBlockApp, &QAbstractButton::clicked, [&] {
        updateCurrentApp(true);
    });

    setupLogBlocked();

    layout->addWidget(m_btAddApp);
    layout->addWidget(m_btEditApp);
    layout->addWidget(m_btDeleteApp);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btAllowApp);
    layout->addWidget(m_btBlockApp);
    layout->addStretch();
    layout->addWidget(m_cbLogBlocked);

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

    // App Group
    setupComboAppGroups();

    formLayout->addRow("Application Group:", m_comboAppGroup);
    m_labelAppGroup = qobject_cast<QLabel *>(formLayout->labelForField(m_comboAppGroup));

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
    m_btEditCancel = new QPushButton(QIcon(":/images/cancel.png"), QString());

    buttonsLayout->addWidget(m_btEditOk, 1, Qt::AlignRight);
    buttonsLayout->addWidget(m_btEditCancel);

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
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

        const int groupIndex = m_comboAppGroup->currentIndex();
        const bool blocked = m_rbBlockApp->isChecked();

        QDateTime endTime;
        if (!blocked && m_cscBlockApp->checkBox()->isChecked()) {
            const int hours = m_cscBlockApp->spinBox()->value();

            endTime = QDateTime::currentDateTime()
                    .addSecs(hours * 60 * 60);
        }

        if (m_formAppId != 0
                ? appListModel()->updateApp(m_formAppId, appPath,
                                            groupIndex, blocked, endTime)
                : appListModel()->addApp(appPath, groupIndex, blocked, endTime)) {
            m_formAppEdit->close();
        }
    });
    connect(m_btEditCancel, &QAbstractButton::clicked, m_formAppEdit, &QWidget::close);
}

void ProgramsWindow::setupComboAppGroups()
{
    m_comboAppGroup = new QComboBox();

    const auto refreshComboAppGroups = [&] {
        QStringList list;
        for (const auto appGroup : conf()->appGroups()) {
            list.append(appGroup->name());
        }

        m_comboAppGroup->clear();
        m_comboAppGroup->addItems(list);
        m_comboAppGroup->setCurrentIndex(0);
    };

    refreshComboAppGroups();

    connect(fortManager(), &FortManager::confChanged, this, refreshComboAppGroups);
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
    m_appListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_appListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_appListView->setModel(appListModel());

    connect(m_appListView, &TableView::doubleClicked, m_btEditApp, &QPushButton::click);
}

void ProgramsWindow::setupTableAppsHeader()
{
    auto header = m_appListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    header->setSectionResizeMode(4, QHeaderView::Interactive);
}

void ProgramsWindow::setupAppInfoRow()
{
    auto layout = new QHBoxLayout();
    layout->setMargin(0);

    m_btAppCopyPath = ControlUtil::createLinkButton(":/images/page_copy.png");
    m_btAppOpenFolder = ControlUtil::createLinkButton(":/images/folder_go.png");

    m_labelAppPath = new QLabel();
    m_labelAppPath->setWordWrap(true);

    m_labelAppProductName = new QLabel();
    m_labelAppProductName->setFont(ControlUtil::fontDemiBold());

    m_labelAppCompanyName = new QLabel();

    connect(m_btAppCopyPath, &QAbstractButton::clicked, [&] {
        GuiUtil::setClipboardData(appListCurrentPath());
    });
    connect(m_btAppOpenFolder, &QAbstractButton::clicked, [&] {
        OsUtil::openFolder(appListCurrentPath());
    });

    layout->addWidget(m_btAppCopyPath);
    layout->addWidget(m_btAppOpenFolder);
    layout->addWidget(m_labelAppPath, 1);
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

        m_labelAppPath->setText(appPath);

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
        m_btEditApp->setEnabled(appSelected);
        m_btDeleteApp->setEnabled(appSelected);
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

        appRow = appListModel()->appRow(appIndex);
        m_formAppId = appRow.appId;
    } else {
        m_formAppId = 0;
    }

    m_editPath->setText(appRow.appPath);
    m_editPath->setReadOnly(editCurrentApp);
    m_btSelectFile->setEnabled(!editCurrentApp);
    m_comboAppGroup->setCurrentIndex(appRow.groupIndex);
    m_rbAllowApp->setChecked(!appRow.blocked());
    m_rbBlockApp->setChecked(appRow.blocked());
    m_cscBlockApp->setEnabled(!appRow.blocked());
    m_cscBlockApp->checkBox()->setChecked(false);
    m_formAppEdit->show();
}

void ProgramsWindow::updateCurrentApp(bool blocked)
{
    const int appIndex = appListCurrentIndex();
    if (appIndex >= 0) {
        const auto appRow = appListModel()->appRow(appIndex);
        appListModel()->updateApp(appRow.appId, appRow.appPath,
                                  appRow.groupIndex, blocked);
    }
}

void ProgramsWindow::deleteCurrentApp()
{
    const int appIndex = appListCurrentIndex();
    if (appIndex >= 0) {
        const auto appRow = appListModel()->appRow(appIndex);
        appListModel()->deleteApp(appRow.appId, appRow.appPath);
    }
}

int ProgramsWindow::appListCurrentIndex() const
{
    return m_appListView->currentIndex().row();
}

QString ProgramsWindow::appListCurrentPath() const
{
    return appListModel()->appPathByRow(appListCurrentIndex());
}

FortManager *ProgramsWindow::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *ProgramsWindow::settings() const
{
    return ctrl()->settings();
}

FirewallConf *ProgramsWindow::conf() const
{
    return ctrl()->conf();
}

AppInfoCache *ProgramsWindow::appInfoCache() const
{
    return appListModel()->appInfoCache();
}
