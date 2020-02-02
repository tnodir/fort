#include "zoneswindow.h"

#include <QVBoxLayout>

#include "../../conf/confmanager.h"
#include "../../fortmanager.h"
#include "../../model/zonelistmodel.h"
#include "../../util/guiutil.h"
#include "zonescontroller.h"

ZonesWindow::ZonesWindow(FortManager *fortManager,
                         QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ZonesController(fortManager, this))
{
    setupUi();
    setupController();
}

void ZonesWindow::setupController()
{
    connect(ctrl(), &ZonesController::retranslateUi, this, &ZonesWindow::onRetranslateUi);

    connect(this, &ZonesWindow::aboutToClose,
            fortManager(), &FortManager::closeZonesWindow);

    connect(fortManager(), &FortManager::afterSaveProgWindowState,
            this, &ZonesWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreProgWindowState,
            this, &ZonesWindow::onRestoreWindowState);

    emit ctrl()->retranslateUi();
}

void ZonesWindow::onSaveWindowState()
{
}

void ZonesWindow::onRestoreWindowState()
{
}

void ZonesWindow::onRetranslateUi()
{
    this->setWindowTitle(tr("Zones"));
}

void ZonesWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);


    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png",
                                             ":/images/map.png"));

    // Size
    this->resize(1024, 768);
    this->setMinimumSize(500, 400);
}

FortManager *ZonesWindow::fortManager() const
{
    return ctrl()->fortManager();
}

ConfManager *ZonesWindow::confManager() const
{
    return ctrl()->confManager();
}

ZoneListModel *ZonesWindow::zoneListModel() const
{
    return fortManager()->zoneListModel();
}
