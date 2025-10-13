#include "serviceswindow.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <manager/serviceinfomanager.h>
#include <manager/windowmanager.h>
#include <model/servicelistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "servicescontroller.h"

using namespace Fort;

namespace {

constexpr int SERVICES_HEADER_VERSION = 2;

}

ServicesWindow::ServicesWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new ServicesController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::serviceWindowGroup());
}

ServiceListModel *ServicesWindow::serviceListModel() const
{
    return ctrl()->serviceListModel();
}

void ServicesWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setServiceWindowGeometry(stateWatcher()->geometry());
    iniUser.setServiceWindowMaximized(stateWatcher()->maximized());

    auto header = m_serviceListView->horizontalHeader();
    iniUser.setServicesHeader(header->saveState());
    iniUser.setServicesHeaderVersion(SERVICES_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ServicesWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(this, QSize(800, 600), iniUser.serviceWindowGeometry(),
            iniUser.serviceWindowMaximized());

    if (iniUser.servicesHeaderVersion() == SERVICES_HEADER_VERSION) {
        auto header = m_serviceListView->horizontalHeader();
        header->restoreState(iniUser.servicesHeader());
    }
}

void ServicesWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actTrack->setText(tr("Make Trackable"));
    m_actRevert->setText(tr("Revert Changes"));
    m_actAddProgram->setText(tr("Add Program"));

    m_btTrack->setText(tr("Make Trackable"));
    m_btRevert->setText(tr("Revert Changes"));
    m_btRefresh->setText(tr("Refresh"));

    this->setWindowTitle(tr("Services"));
}

void ServicesWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &ServicesController::retranslateUi, this, &ServicesWindow::retranslateUi);

    retranslateUi();
}

void ServicesWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableServiceList();
    setupTableServiceListHeader();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_serviceListView, 1);

    this->setLayout(layout);

    // Actions on conns table's current changed
    setupTableServicesChanged();

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ServicesWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actTrack = editMenu->addAction(IconCache::icon(":/icons/tick.png"), QString());
    m_actRevert = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());

    m_actAddProgram = editMenu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    connect(m_actTrack, &QAction::triggered, this, [&] {
        const int serviceIndex = serviceListCurrentIndex();
        if (serviceIndex < 0)
            return;

        const auto &serviceInfo = serviceListModel()->serviceInfoAt(serviceIndex);

        serviceInfoManager()->trackService(serviceInfo.serviceName);
        updateServiceListModel();

        windowManager()->showInfoBox(tr("Please restart the computer to reload changed services!"));
    });
    connect(m_actRevert, &QAction::triggered, this, [&] {
        const int serviceIndex = serviceListCurrentIndex();
        if (serviceIndex < 0)
            return;

        const auto &serviceInfo = serviceListModel()->serviceInfoAt(serviceIndex);

        serviceInfoManager()->revertService(serviceInfo.serviceName);
        updateServiceListModel();
    });
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const int serviceIndex = serviceListCurrentIndex();
        if (serviceIndex < 0)
            return;

        const auto &serviceInfo = serviceListModel()->serviceInfoAt(serviceIndex);

        const QString appPath = QStringLiteral(R"(\SvcHost\)") + serviceInfo.serviceName;

        windowManager()->openProgramEditForm(appPath, /*appId=*/0, this);
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Toolbar buttons
    m_btTrack = ControlUtil::createFlatToolButton(":/icons/tick.png");
    m_btRevert = ControlUtil::createFlatToolButton(":/icons/delete.png");
    m_btRefresh = ControlUtil::createFlatToolButton(":/icons/arrow_refresh_small.png");

    connect(m_btTrack, &QAbstractButton::clicked, m_actTrack, &QAction::trigger);
    connect(m_btRevert, &QAbstractButton::clicked, m_actRevert, &QAction::trigger);
    connect(m_btRefresh, &QAbstractButton::clicked, this, &ServicesWindow::updateServiceListModel);

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_btTrack, m_btRevert, ControlUtil::createVSeparator(), m_btRefresh,
            /*stretch*/ nullptr, m_btOptions, m_btMenu });

    return layout;
}

void ServicesWindow::setupTableServiceList()
{
    m_serviceListView = new TableView();
    m_serviceListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_serviceListView->setModel(serviceListModel());

    m_serviceListView->setMenu(m_btEdit->menu());

    connect(m_serviceListView, &TableView::doubleClicked, m_actAddProgram, &QAction::trigger);
}

void ServicesWindow::setupTableServiceListHeader()
{
    auto header = m_serviceListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(0, 180);
    header->resizeSection(1, 520);
    header->resizeSection(2, 100);
}

void ServicesWindow::setupTableServicesChanged()
{
    const auto refreshTableServicesChanged = [&] {
        const int serviceIndex = serviceListCurrentIndex();
        const bool serviceSelected = (serviceIndex >= 0);
        const auto &serviceInfo = serviceListModel()->serviceInfoAt(serviceIndex);

        m_actTrack->setEnabled(serviceSelected && !serviceInfo.isTracked());
        m_actRevert->setEnabled(serviceSelected && serviceInfo.isTracked());
        m_actAddProgram->setEnabled(serviceSelected);
        m_btTrack->setEnabled(m_actTrack->isEnabled());
        m_btRevert->setEnabled(m_actRevert->isEnabled());
    };

    refreshTableServicesChanged();

    connect(m_serviceListView, &TableView::currentIndexChanged, this, refreshTableServicesChanged);
}

void ServicesWindow::updateServiceListModel()
{
    serviceListModel()->initialize();
}

int ServicesWindow::serviceListCurrentIndex() const
{
    return m_serviceListView->currentRow();
}
