#include "programswindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../log/model/applistmodel.h"
#include "../../util/app/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/osutil.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "programscontroller.h"

ProgramsWindow::ProgramsWindow(FortManager *fortManager,
                               QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ProgramsController(fortManager, this)),
    m_appListModel(ctrl()->appListModel())
{
    setupController();
    setupAppListModel();

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

void ProgramsWindow::setupAppListModel()
{
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
    m_cbLogBlocked->setText(tr("Alert about Unknown Programs"));

    appListModel()->refresh();

    m_btAppCopyPath->setToolTip(tr("Copy Path"));
    m_btAppOpenFolder->setToolTip(tr("Open Folder"));
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
    setupAppInfoVersion();
    layout->addWidget(m_appInfoRow);

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

    setupLogBlocked();

    layout->addStretch();
    layout->addWidget(m_cbLogBlocked);

    return layout;
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
