#include "serviceswindow.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/opt/optionscontroller.h>
#include <manager/windowmanager.h>
#include <model/servicelistmodel.h>
#include <serviceinfo/serviceinfomanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "servicescontroller.h"

namespace {

#define SERVICES_HEADER_VERSION 1

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

ServiceListModel *ServicesWindow::serviceListModel() const
{
    return ctrl()->serviceListModel();
}

void ServicesWindow::saveWindowState()
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

    m_btRefresh->setText(tr("Refresh"));
    m_btEdit->setText(tr("Edit"));
    m_actEditService->setText(tr("Edit Service"));
    m_actAddProgram->setText(tr("Add Program"));

    serviceListModel()->refresh();

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
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/windows-48.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ServicesWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_btRefresh = ControlUtil::createButton(
            ":/icons/arrow_refresh_small.png", [&] { serviceListModel()->initialize(); });

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actEditService = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditService->setShortcut(Qt::Key_Return);

    m_actAddProgram = editMenu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(Qt::Key_Insert);

    connect(m_actEditService, &QAction::triggered, this, [&] {
        // const auto connIndex = serviceListCurrentIndex();
        // const auto connRow = serviceListModel()->connRowAt(connIndex);

        // showServiceEditForm(connRow.appPath);
    });
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        const auto serviceIndex = serviceListCurrentIndex();
        const auto serviceInfo = serviceListModel()->serviceInfoAt(serviceIndex);

        const QString appPath = QStringLiteral(R"(\SvcHost\)") + serviceInfo.serviceName;

        windowManager()->showProgramEditForm(appPath);
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    layout->addWidget(m_btEdit);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btRefresh);
    layout->addStretch();

    return layout;
}

void ServicesWindow::setupTableServiceList()
{
    m_serviceListView = new TableView();
    m_serviceListView->setAlternatingRowColors(true);
    m_serviceListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_serviceListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_serviceListView->setModel(serviceListModel());

    m_serviceListView->setMenu(m_btEdit->menu());
}

void ServicesWindow::setupTableServiceListHeader()
{
    auto header = m_serviceListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);

    header->resizeSection(0, 180);
    header->resizeSection(1, 520);
    header->resizeSection(2, 100);
}

void ServicesWindow::setupTableServicesChanged()
{
    const auto refreshTableServicesChanged = [&] {
        const int serviceIndex = serviceListCurrentIndex();
        const bool serviceSelected = (serviceIndex >= 0);
        m_actEditService->setEnabled(serviceSelected);
        m_actAddProgram->setEnabled(serviceSelected);
    };

    refreshTableServicesChanged();

    connect(m_serviceListView, &TableView::currentIndexChanged, this, refreshTableServicesChanged);
}

int ServicesWindow::serviceListCurrentIndex() const
{
    return m_serviceListView->currentRow();
}
