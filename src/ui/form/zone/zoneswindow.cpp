#include "zoneswindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/dialog/dialogutil.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "zoneeditdialog.h"
#include "zonescontroller.h"

namespace {

constexpr int ZONES_HEADER_VERSION = 3;

}

ZonesWindow::ZonesWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ZonesController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *ZonesWindow::confManager() const
{
    return ctrl()->confManager();
}

IniOptions *ZonesWindow::ini() const
{
    return ctrl()->ini();
}

IniUser *ZonesWindow::iniUser() const
{
    return ctrl()->iniUser();
}

WindowManager *ZonesWindow::windowManager() const
{
    return ctrl()->windowManager();
}

TaskManager *ZonesWindow::taskManager() const
{
    return ctrl()->taskManager();
}

ZoneListModel *ZonesWindow::zoneListModel() const
{
    return ctrl()->zoneListModel();
}

void ZonesWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setZoneWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setZoneWindowMaximized(m_stateWatcher->maximized());

    auto header = m_zoneListView->horizontalHeader();
    iniUser()->setZonesHeader(header->saveState());
    iniUser()->setZonesHeaderVersion(ZONES_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ZonesWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(900, 400), iniUser()->zoneWindowGeometry(),
            iniUser()->zoneWindowMaximized());

    if (iniUser()->zonesHeaderVersion() == ZONES_HEADER_VERSION) {
        auto header = m_zoneListView->horizontalHeader();
        header->restoreState(iniUser()->zonesHeader());
    }
}

void ZonesWindow::setupController()
{
    connect(ctrl(), &ZonesController::retranslateUi, this, &ZonesWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void ZonesWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void ZonesWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddZone->setText(tr("Add"));
    m_actEditZone->setText(tr("Edit"));
    m_actRemoveZone->setText(tr("Remove"));
    m_btSaveAsText->setText(tr("Save As Text"));
    m_btUpdateZones->setText(tr("Update Zones"));

    zoneListModel()->refresh();

    this->setWindowTitle(tr("Zones"));
}

void ZonesWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableZones();
    setupTableZonesHeader();

    // Actions on zones table's current changed
    setupTableZonesChanged();

    // Actions on zone list model's changed
    setupZoneListModelChanged();

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addLayout(header);
    layout->addWidget(m_zoneListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/ip_class.png"));

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *ZonesWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddZone = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddZone->setShortcut(Qt::Key_Plus);

    m_actEditZone = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditZone->setShortcut(Qt::Key_Enter);

    m_actRemoveZone = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveZone->setShortcut(Qt::Key_Delete);

    connect(m_actAddZone, &QAction::triggered, this, &ZonesWindow::addNewZone);
    connect(m_actEditZone, &QAction::triggered, this, &ZonesWindow::editSelectedZone);
    connect(m_actRemoveZone, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { deleteSelectedZone(); }, tr("Are you sure to remove selected zone?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Save As Text
    setupSaveAsText();

    // Run Task
    setupTaskRun();

    // Menu button
    m_btMenu = windowManager()->createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_btSaveAsText, ControlUtil::createVSeparator(), m_btUpdateZones,
            /*stretch*/ nullptr, m_btMenu });

    return layout;
}

void ZonesWindow::setupSaveAsText()
{
    m_btSaveAsText = ControlUtil::createFlatToolButton(":/icons/save_as.png", [&] {
        const auto filePath = DialogUtil::getSaveFileName(
                m_btSaveAsText->text(), tr("Text files (*.txt);;All files (*.*)"));
        if (filePath.isEmpty())
            return;

        const int zoneIndex = zoneListCurrentIndex();
        if (zoneIndex >= 0
                && !taskManager()->taskInfoZoneDownloader()->saveZoneAsText(filePath, zoneIndex)) {
            windowManager()->showErrorBox(tr("Cannot save Zone addresses as text file"));
        }
    });
}

void ZonesWindow::setupTaskRun()
{
    m_btUpdateZones = ControlUtil::createFlatToolButton(
            ":/icons/play.png", [&] { taskManager()->runTask(TaskInfo::ZoneDownloader); });
}

void ZonesWindow::setupTableZones()
{
    m_zoneListView = new TableView();
    m_zoneListView->setAlternatingRowColors(true);
    m_zoneListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_zoneListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_zoneListView->setModel(zoneListModel());

    m_zoneListView->setMenu(m_btEdit->menu());

    connect(m_zoneListView, &TableView::activated, m_actEditZone, &QAction::trigger);
}

void ZonesWindow::setupTableZonesHeader()
{
    auto header = m_zoneListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Stretch);

    header->resizeSection(0, 250);
    header->resizeSection(1, 350);
    header->resizeSection(2, 90);
}

void ZonesWindow::setupTableZonesChanged()
{
    const auto refreshTableZonesChanged = [&] {
        const int zoneIndex = zoneListCurrentIndex();
        const bool zoneSelected = (zoneIndex >= 0);
        m_actEditZone->setEnabled(zoneSelected);
        m_actRemoveZone->setEnabled(zoneSelected);
        m_btSaveAsText->setEnabled(zoneSelected);
    };

    refreshTableZonesChanged();

    connect(m_zoneListView, &TableView::currentIndexChanged, this, refreshTableZonesChanged);
}

void ZonesWindow::setupZoneListModelChanged()
{
    const auto refreshAddZone = [&] {
        m_actAddZone->setEnabled(zoneListModel()->rowCount() < ConfUtil::zoneMaxCount());
    };

    refreshAddZone();

    connect(zoneListModel(), &ZoneListModel::modelReset, this, refreshAddZone);
    connect(zoneListModel(), &ZoneListModel::rowsRemoved, this, refreshAddZone);
}

void ZonesWindow::addNewZone()
{
    openZoneEditForm({});
}

void ZonesWindow::editSelectedZone()
{
    const int zoneIndex = zoneListCurrentIndex();
    if (zoneIndex < 0)
        return;

    const auto &zoneRow = zoneListModel()->zoneRowAt(zoneIndex);

    openZoneEditForm(zoneRow);
}

void ZonesWindow::openZoneEditForm(const ZoneRow &zoneRow)
{
    if (!m_formZoneEdit) {
        m_formZoneEdit = new ZoneEditDialog(ctrl(), this);
    }

    m_formZoneEdit->initialize(zoneRow);

    WidgetWindow::showWidget(m_formZoneEdit);
}

void ZonesWindow::deleteZone(int row)
{
    const auto &zoneRow = zoneListModel()->zoneRowAt(row);
    if (zoneRow.isNull())
        return;

    ctrl()->deleteZone(zoneRow.zoneId);
}

void ZonesWindow::deleteSelectedZone()
{
    deleteZone(zoneListCurrentIndex());
}

int ZonesWindow::zoneListCurrentIndex() const
{
    return m_zoneListView->currentRow();
}
