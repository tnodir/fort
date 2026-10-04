#include "serviceswindow.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
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

inline constexpr int SERVICES_HEADER_VERSION = 2;

inline constexpr int SERVICE_RESTART_REFRESH_DELAY = 3000; // msec

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
    m_actRestart->setText(tr("Restart Service"));
    m_actAddProgram->setText(tr("Add Program"));
    m_actFind->setText(tr("Find"));

    m_btTrack->setText(tr("Make Trackable"));
    m_btRevert->setText(tr("Revert Changes"));
    m_btRestart->setText(tr("Restart Service"));
    m_btRefresh->setText(tr("Refresh"));

    m_editSearch->setPlaceholderText(tr("Search") + " /");

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
    m_actRestart =
            editMenu->addAction(IconCache::icon(":/icons/arrow_rotate_clockwise.png"), QString());

    m_actAddProgram = editMenu->addAction(IconCache::icon(":/icons/application.png"), QString());
    m_actAddProgram->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    editMenu->addSeparator();

    m_actFind = editMenu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actFind->setShortcut(QKeySequence::Find);

    connect(m_actTrack, &QAction::triggered, this, [&] {
        if (const auto serviceInfo = currentServiceInfo()) {
            trackService(*serviceInfo);
        }
    });
    connect(m_actRevert, &QAction::triggered, this, [&] {
        if (const auto serviceInfo = currentServiceInfo()) {
            serviceInfoManager()->revertService(serviceInfo->serviceName);
            updateServiceListModel();
        }
    });
    connect(m_actRestart, &QAction::triggered, this, [&] {
        if (const auto serviceInfo = currentServiceInfo()) {
            confirmRestartService(serviceInfo->realServiceName, tr("Restart the service \"%1\"?"));
        }
    });
    connect(m_actAddProgram, &QAction::triggered, this, [&] {
        if (const auto serviceInfo = currentServiceInfo()) {
            const QString appPath = QStringLiteral(R"(\SvcHost\)") + serviceInfo->serviceName;

            windowManager()->openProgramEditForm(appPath, /*appId=*/0, this);
        }
    });
    connect(m_actFind, &QAction::triggered, this, [&] {
        m_editSearch->setFocus();
        m_editSearch->selectAll();
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Toolbar buttons
    m_btTrack = ControlUtil::createFlatToolButton(":/icons/tick.png");
    m_btRevert = ControlUtil::createFlatToolButton(":/icons/delete.png");
    m_btRestart = ControlUtil::createFlatToolButton(":/icons/arrow_rotate_clockwise.png");
    m_btRefresh = ControlUtil::createFlatToolButton(":/icons/arrow_refresh_small.png");

    connect(m_btTrack, &QAbstractButton::clicked, m_actTrack, &QAction::trigger);
    connect(m_btRevert, &QAbstractButton::clicked, m_actRevert, &QAction::trigger);
    connect(m_btRestart, &QAbstractButton::clicked, m_actRestart, &QAction::trigger);
    connect(m_btRefresh, &QAbstractButton::clicked, this, &ServicesWindow::updateServiceListModel);

    // Search field
    setupEditSearch();

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btEdit, ControlUtil::createVSeparator(), m_btTrack, m_btRevert, m_btRestart,
                    ControlUtil::createVSeparator(), m_btRefresh, ControlUtil::createVSeparator(),
                    m_editSearch, /*stretch*/ nullptr, m_btOptions, m_btMenu });

    return layout;
}

void ServicesWindow::setupEditSearch()
{
    m_editSearch = ControlUtil::createLineEdit(
            QString(), [&](const QString &text) { serviceListModel()->setTextFilter(text); });
    m_editSearch->setClearButtonEnabled(true);
    m_editSearch->setMaxLength(200);
    m_editSearch->setMinimumWidth(100);
    m_editSearch->setMaximumWidth(200);

    connect(this, &ServicesWindow::aboutToShow, m_editSearch, qOverload<>(&QWidget::setFocus));
}

void ServicesWindow::setupTableServiceList()
{
    m_serviceListView = new TableView();
    m_serviceListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_serviceListView->setSortingEnabled(true);
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

    header->setSectionsClickable(true);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(0, Qt::AscendingOrder);
}

void ServicesWindow::setupTableServicesChanged()
{
    const auto refreshTableServicesChanged = [&] {
        const auto serviceInfo = currentServiceInfo();
        const bool serviceSelected = (serviceInfo != nullptr);

        m_actTrack->setEnabled(serviceSelected && serviceInfo->canTrack());
        m_actRevert->setEnabled(serviceSelected && serviceInfo->isTracked());
        m_actRestart->setEnabled(serviceSelected && serviceInfo->isStoppable);
        m_actAddProgram->setEnabled(serviceSelected);
        m_btTrack->setEnabled(m_actTrack->isEnabled());
        m_btRevert->setEnabled(m_actRevert->isEnabled());
        m_btRestart->setEnabled(m_actRestart->isEnabled());
    };

    refreshTableServicesChanged();

    connect(m_serviceListView, &TableView::currentIndexChanged, this, refreshTableServicesChanged);
}

// The serviceInfo is copied: the services list is reloaded
void ServicesWindow::trackService(ServiceInfo serviceInfo)
{
    serviceInfoManager()->trackService(serviceInfo.serviceName);
    updateServiceListModel();

    // The running service's process doesn't have the changes
    if (!serviceInfo.isProcessShared)
        return;

    if (serviceInfo.isRestartable()) {
        confirmRestartService(serviceInfo.realServiceName,
                tr("Restart the service \"%1\" to apply the changes?"));
    } else {
        windowManager()->showInfoBox(tr("Please restart the computer to apply the changes."));
    }
}

void ServicesWindow::confirmRestartService(const QString &serviceName, const QString &question)
{
    windowManager()->showConfirmBox(
            [=, this] { restartService(serviceName); }, question.arg(serviceName), QString(), this);
}

void ServicesWindow::restartService(const QString &serviceName)
{
    if (!serviceInfoManager()->restartService(serviceName)) {
        windowManager()->showErrorBox(
                tr("Cannot restart the service \"%1\".").arg(serviceName), QString(), this);
        return;
    }

    updateServiceListModel();

    // Refresh the list when the service is started
    QTimer::singleShot(
            SERVICE_RESTART_REFRESH_DELAY, this, &ServicesWindow::updateServiceListModel);
}

void ServicesWindow::updateServiceListModel()
{
    serviceListModel()->initialize();
}

const ServiceInfo *ServicesWindow::currentServiceInfo() const
{
    const int serviceIndex = serviceListCurrentIndex();
    if (serviceIndex < 0)
        return nullptr;

    return &serviceListModel()->serviceInfoAt(serviceIndex);
}

int ServicesWindow::serviceListCurrentIndex() const
{
    return m_serviceListView->currentRow();
}
