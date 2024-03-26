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
#include <manager/serviceinfomanager.h>
#include <manager/windowmanager.h>
#include <model/servicelistmodel.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "servicescontroller.h"

namespace {

constexpr int SERVICES_HEADER_VERSION = 2;

}

ServicesWindow::ServicesWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ServicesController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *ServicesWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *ServicesWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *ServicesWindow::windowManager() const
{
    return ctrl()->windowManager();
}

ServiceInfoManager *ServicesWindow::serviceInfoManager() const
{
    return ctrl()->serviceInfoManager();
}

ServiceListModel *ServicesWindow::serviceListModel() const
{
    return ctrl()->serviceListModel();
}

void ServicesWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setServiceWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setServiceWindowMaximized(m_stateWatcher->maximized());

    auto header = m_serviceListView->horizontalHeader();
    iniUser()->setServicesHeader(header->saveState());
    iniUser()->setServicesHeaderVersion(SERVICES_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ServicesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(800, 600), iniUser()->serviceWindowGeometry(),
            iniUser()->serviceWindowMaximized());

    if (iniUser()->servicesHeaderVersion() == SERVICES_HEADER_VERSION) {
        auto header = m_serviceListView->horizontalHeader();
        header->restoreState(iniUser()->servicesHeader());
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

void ServicesWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void ServicesWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableServiceList();
    setupTableServiceListHeader();
    layout->addWidget(m_serviceListView, 1);

    this->setLayout(layout);

    // Actions on conns table's current changed
    setupTableServicesChanged();

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/windows-48.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ServicesWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actTrack = editMenu->addAction(IconCache::icon(":/icons/widgets.png"), QString());
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

        windowManager()->showProgramEditForm(appPath);
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Toolbar buttons
    m_btTrack = ControlUtil::createFlatToolButton(":/icons/widgets.png");
    m_btRevert = ControlUtil::createFlatToolButton(":/icons/delete.png");
    m_btRefresh = ControlUtil::createFlatToolButton(":/icons/arrow_refresh_small.png");

    connect(m_btTrack, &QAbstractButton::clicked, m_actTrack, &QAction::trigger);
    connect(m_btRevert, &QAbstractButton::clicked, m_actRevert, &QAction::trigger);
    connect(m_btRefresh, &QAbstractButton::clicked, this, &ServicesWindow::updateServiceListModel);

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btTrack);
    layout->addWidget(m_btRevert);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btRefresh);
    layout->addStretch();

    return layout;
}

void ServicesWindow::setupTableServiceList()
{
    m_serviceListView = new TableView();
    m_serviceListView->setAlternatingRowColors(true);
    m_serviceListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_serviceListView->setModel(serviceListModel());

    m_serviceListView->setMenu(m_btEdit->menu());
}

void ServicesWindow::setupTableServiceListHeader()
{
    auto header = m_serviceListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Stretch);

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
