#include "zoneswindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/dialog/dialogutil.h>
#include <fortglobal.h>
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

using namespace Fort;

namespace {

constexpr int ZONES_HEADER_VERSION = 3;

}

ZonesWindow::ZonesWindow(QWidget *parent) : FormWindow(parent), m_ctrl(new ZonesController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::zoneWindowGroup());
}

ZoneListModel *ZonesWindow::zoneListModel() const
{
    return ctrl()->zoneListModel();
}

void ZonesWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setZoneWindowGeometry(stateWatcher()->geometry());
    iniUser.setZoneWindowMaximized(stateWatcher()->maximized());

    auto header = m_zoneListView->horizontalHeader();
    iniUser.setZonesHeader(header->saveState());
    iniUser.setZonesHeaderVersion(ZONES_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ZonesWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(900, 400), iniUser.zoneWindowGeometry(), iniUser.zoneWindowMaximized());

    if (iniUser.zonesHeaderVersion() == ZONES_HEADER_VERSION) {
        auto header = m_zoneListView->horizontalHeader();
        header->restoreState(iniUser.zonesHeader());
    }
}

void ZonesWindow::setupController()
{
    connect(ctrl(), &ZonesController::retranslateUi, this, &ZonesWindow::retranslateUi);

    emit ctrl()->retranslateUi();
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

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_zoneListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

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

    // Options button
    m_btOptions = ControlUtil::createOptionsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btEdit, ControlUtil::createVSeparator(),
            m_btSaveAsText, ControlUtil::createVSeparator(), m_btUpdateZones,
            /*stretch*/ nullptr, m_btOptions, m_btMenu });

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
    m_btUpdateZones =
            ControlUtil::createFlatToolButton(":/icons/play.png", [&] { downloadZones(); });
}

void ZonesWindow::setupTableZones()
{
    m_zoneListView = new TableView();
    m_zoneListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_zoneListView->setSelectionBehavior(QAbstractItemView::SelectRows);

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

void ZonesWindow::openZoneEditForm(const Zone &zone)
{
    if (!m_formZoneEdit) {
        m_formZoneEdit = new ZoneEditDialog(ctrl(), this);

        connect(m_formZoneEdit, &ZoneEditDialog::saved, [&] {
            windowManager()->showConfirmBox([&] { downloadZones(); }, tr("Update Zones?"));
        });
    }

    m_formZoneEdit->initialize(zone);

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

void ZonesWindow::downloadZones()
{
    taskManager()->runTask(TaskInfo::ZoneDownloader);
}

int ZonesWindow::zoneListCurrentIndex() const
{
    return m_zoneListView->currentRow();
}
