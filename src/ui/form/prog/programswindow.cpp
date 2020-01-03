#include "programswindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../log/model/applistmodel.h"
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
    auto header = m_tableApps->horizontalHeader();
    settings()->setProgAppsHeader(header->saveState());
}

void ProgramsWindow::onRestoreWindowState()
{
    auto header = m_tableApps->horizontalHeader();
    header->restoreState(settings()->progAppsHeader());
}

void ProgramsWindow::onRetranslateUi()
{
    m_cbLogBlocked->setText(tr("Alert Blocked Programs"));

    appListModel()->refresh();
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
    layout->addWidget(m_tableApps, 1);

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
    m_cbLogBlocked = ControlUtil::createCheckBox(false, [&](bool checked) {
        if (conf()->logBlocked() == checked)
            return;

        conf()->setLogBlocked(checked);

        fortManager()->applyConfImmediateFlags();
    });

    m_cbLogBlocked->setFont(ControlUtil::fontDemiBold());
}

void ProgramsWindow::setupTableApps()
{
    m_tableApps = new TableView();

    m_tableApps->setModel(appListModel());
}

void ProgramsWindow::setupTableAppsHeader()
{
    auto header = m_tableApps->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
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
