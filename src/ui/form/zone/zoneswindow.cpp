#include "zoneswindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../model/zonelistmodel.h"
#include "../../model/zonesourcewrapper.h"
#include "../../task/taskinfozonedownloader.h"
#include "../../task/taskmanager.h"
#include "../../util/conf/confutil.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../../util/window/widgetwindowstatewatcher.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "zonescontroller.h"

namespace {

#define ZONES_HEADER_VERSION 1

}

ZonesWindow::ZonesWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new ZonesController(fortManager, this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

FortManager *ZonesWindow::fortManager() const
{
    return ctrl()->fortManager();
}

ConfManager *ZonesWindow::confManager() const
{
    return ctrl()->confManager();
}

IniOptions *ZonesWindow::ini() const
{
    return ctrl()->ini();
}

TaskManager *ZonesWindow::taskManager() const
{
    return fortManager()->taskManager();
}

ZoneListModel *ZonesWindow::zoneListModel() const
{
    return fortManager()->zoneListModel();
}

void ZonesWindow::saveWindowState()
{
    ini()->setZoneWindowGeometry(m_stateWatcher->geometry());
    ini()->setZoneWindowMaximized(m_stateWatcher->maximized());

    auto header = m_zoneListView->horizontalHeader();
    ini()->setZonesHeader(header->saveState());
    ini()->setZonesHeaderVersion(ZONES_HEADER_VERSION);

    confManager()->saveIni();
}

void ZonesWindow::restoreWindowState()
{
    m_stateWatcher->restore(
            this, QSize(1024, 768), ini()->zoneWindowGeometry(), ini()->zoneWindowMaximized());

    if (ini()->zonesHeaderVersion() == ZONES_HEADER_VERSION) {
        auto header = m_zoneListView->horizontalHeader();
        header->restoreState(ini()->zonesHeader());
    }
}

void ZonesWindow::setupController()
{
    connect(ctrl(), &ZonesController::retranslateUi, this, &ZonesWindow::onRetranslateUi);

    emit ctrl()->retranslateUi();
}

void ZonesWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void ZonesWindow::onRetranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddZone->setText(tr("Add"));
    m_actEditZone->setText(tr("Edit"));
    m_actRemoveZone->setText(tr("Remove"));
    m_btSaveAsText->setText(tr("Save As Text"));

    m_formZoneEdit->unsetLocale();
    m_formZoneEdit->setWindowTitle(tr("Edit Zone"));

    m_labelZoneName->setText(tr("Zone Name:"));
    m_labelSource->setText(tr("Source:"));
    m_cbEnabled->setText(tr("Enabled"));
    m_cbCustomUrl->setText(tr("Custom URL"));
    m_labelUrl->setText(tr("URL:"));
    m_labelFormData->setText(tr("Form Data:"));
    m_btEditOk->setText(tr("OK"));
    m_btEditCancel->setText(tr("Cancel"));

    zoneListModel()->refresh();

    this->setWindowTitle(tr("Zones"));
}

void ZonesWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Zone Add/Edit Form
    setupZoneEditForm();

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableZones();
    setupTableZonesHeader();
    layout->addWidget(m_zoneListView, 1);

    // Actions on zones table's current changed
    setupTableZonesChanged();

    // Actions on zone list model's changed
    setupZoneListModelChanged();

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(
            GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/map-map-marker.png"));

    // Size
    this->setMinimumSize(500, 400);
}

void ZonesWindow::setupZoneEditForm()
{
    auto formLayout = new QFormLayout();

    // Zone Name
    m_editZoneName = new QLineEdit();
    m_editZoneName->setMaxLength(256);

    formLayout->addRow("Zone Name:", m_editZoneName);
    m_labelZoneName = qobject_cast<QLabel *>(formLayout->labelForField(m_editZoneName));

    // Sources
    setupComboSources();

    formLayout->addRow("Source:", m_comboSources);
    m_labelSource = qobject_cast<QLabel *>(formLayout->labelForField(m_comboSources));

    // Enabled
    m_cbEnabled = new QCheckBox();

    formLayout->addRow(QString(), m_cbEnabled);

    // Custom URL
    m_cbCustomUrl = new QCheckBox();

    formLayout->addRow(QString(), m_cbCustomUrl);

    // URL
    m_editUrl = new QLineEdit();
    m_editUrl->setEnabled(false);
    m_editUrl->setMaxLength(1024);

    formLayout->addRow("URL:", m_editUrl);
    m_labelUrl = qobject_cast<QLabel *>(formLayout->labelForField(m_editUrl));

    // Form Data
    m_editFormData = new QLineEdit();
    m_editFormData->setEnabled(false);

    formLayout->addRow("Form Data:", m_editFormData);
    m_labelFormData = qobject_cast<QLabel *>(formLayout->labelForField(m_editFormData));

    // OK/Cancel
    auto buttonsLayout = new QHBoxLayout();

    m_btEditOk = new QPushButton();
    m_btEditOk->setDefault(true);

    m_btEditCancel = new QPushButton();

    buttonsLayout->addWidget(m_btEditOk, 1, Qt::AlignRight);
    buttonsLayout->addWidget(m_btEditCancel);

    // Form
    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    m_formZoneEdit = new QDialog(this);
    m_formZoneEdit->setWindowModality(Qt::WindowModal);
    m_formZoneEdit->setSizeGripEnabled(true);
    m_formZoneEdit->setLayout(layout);
    m_formZoneEdit->setMinimumWidth(500);

    connect(m_cbCustomUrl, &QCheckBox::toggled, this, [&](bool checked) {
        m_editUrl->setEnabled(checked);
        m_editFormData->setEnabled(checked);
    });

    connect(m_btEditOk, &QAbstractButton::clicked, this, [&] {
        if (saveZoneEditForm()) {
            m_formZoneEdit->close();
        }
    });
    connect(m_btEditCancel, &QAbstractButton::clicked, m_formZoneEdit, &QWidget::close);
}

void ZonesWindow::setupComboSources()
{
    m_comboSources = new QComboBox();

    const auto refreshComboZoneTypes = [&](bool onlyFlags = false) {
        if (onlyFlags)
            return;

        m_comboSources->clear();
        for (const auto &sourceVar : zoneListModel()->zoneSources()) {
            const ZoneSourceWrapper zoneSource(sourceVar);
            m_comboSources->addItem(zoneSource.title(), sourceVar);
        }
        m_comboSources->setCurrentIndex(0);
    };

    refreshComboZoneTypes();
}

QLayout *ZonesWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actAddZone = editMenu->addAction(IconCache::icon(":/icons/sign-add.png"), QString());
    m_actAddZone->setShortcut(Qt::Key_Plus);

    m_actEditZone = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditZone->setShortcut(Qt::Key_Enter);

    m_actRemoveZone = editMenu->addAction(IconCache::icon(":/icons/sign-delete.png"), QString());
    m_actRemoveZone->setShortcut(Qt::Key_Delete);

    connect(m_actAddZone, &QAction::triggered, this, [&] { updateZoneEditForm(false); });
    connect(m_actEditZone, &QAction::triggered, this, [&] { updateZoneEditForm(true); });
    connect(m_actRemoveZone, &QAction::triggered, this, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove selected zone?"))) {
            deleteSelectedZone();
        }
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Save As Text
    m_btSaveAsText = ControlUtil::createButton(":/icons/floppy.png", [&] {
        const auto filePath = ControlUtil::getSaveFileName(
                m_btSaveAsText->text(), tr("Text files (*.txt);;All files (*.*)"));

        if (!filePath.isEmpty()) {
            const int zoneIndex = zoneListCurrentIndex();

            if (!taskManager()->taskInfoZoneDownloader()->saveZoneAsText(filePath, zoneIndex)) {
                fortManager()->showErrorBox(tr("Cannot save Zone addresses as text file"));
            }
        }
    });

    layout->addWidget(m_btEdit);
    layout->addWidget(m_btSaveAsText);
    layout->addStretch();

    return layout;
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
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    header->resizeSection(0, 350);
    header->resizeSection(1, 290);
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

void ZonesWindow::updateZoneEditForm(bool editCurrentZone)
{
    ZoneRow zoneRow;
    if (editCurrentZone) {
        const auto zoneIndex = zoneListCurrentIndex();
        if (zoneIndex < 0)
            return;

        zoneRow = zoneListModel()->zoneRowAt(zoneIndex);
    }

    const auto zoneSource =
            ZoneSourceWrapper(zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));

    m_formZoneIsNew = !editCurrentZone;

    m_editZoneName->setText(zoneRow.zoneName);
    m_editZoneName->selectAll();
    m_editZoneName->setFocus();
    m_comboSources->setCurrentIndex(zoneSource.index());
    m_cbEnabled->setChecked(zoneRow.enabled);
    m_cbCustomUrl->setChecked(zoneRow.customUrl);
    m_editUrl->setText(zoneRow.url);
    m_editFormData->setText(zoneRow.formData);

    m_formZoneEdit->show();
}

bool ZonesWindow::saveZoneEditForm()
{
    const auto zoneSource = ZoneSourceWrapper(m_comboSources->currentData());
    const auto sourceCode = zoneSource.code();

    if (zoneSource.url().isEmpty()) {
        m_cbCustomUrl->setChecked(true);
    }

    const auto zoneName = m_editZoneName->text();
    const bool enabled = m_cbEnabled->isChecked();
    const bool customUrl = m_cbCustomUrl->isChecked();
    const auto url = m_editUrl->text();
    const auto formData = m_editFormData->text();

    // Check zone name
    if (zoneName.isEmpty()) {
        m_editZoneName->setFocus();
        return false;
    }

    // Check custom url
    if (customUrl && url.isEmpty()) {
        m_editUrl->setText(zoneSource.url());
        m_editUrl->selectAll();
        m_editUrl->setFocus();
        m_editFormData->setText(zoneSource.formData());
        return false;
    }

    // Add new zone
    if (m_formZoneIsNew) {
        int zoneId;
        if (zoneListModel()->addZone(
                    zoneName, sourceCode, url, formData, enabled, customUrl, zoneId)) {
            m_zoneListView->selectCell(zoneId - 1);
            return true;
        }
        return false;
    }

    // Edit selected zone
    const int zoneIndex = zoneListCurrentIndex();
    const auto zoneRow = zoneListModel()->zoneRowAt(zoneIndex);

    const bool zoneNameEdited = (zoneName != zoneRow.zoneName);
    const bool zoneEdited = (sourceCode != zoneRow.sourceCode || enabled != zoneRow.enabled
            || customUrl != zoneRow.customUrl || url != zoneRow.url
            || formData != zoneRow.formData);

    if (!zoneEdited) {
        if (zoneNameEdited) {
            return zoneListModel()->updateZoneName(zoneRow.zoneId, zoneName);
        }
        return true;
    }

    return zoneListModel()->updateZone(
            zoneRow.zoneId, zoneName, sourceCode, url, formData, enabled, customUrl);
}

void ZonesWindow::updateZone(int row, bool enabled)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(row);

    zoneListModel()->updateZone(zoneRow.zoneId, zoneRow.zoneName, zoneRow.sourceCode, zoneRow.url,
            zoneRow.formData, enabled, zoneRow.customUrl);
}

void ZonesWindow::deleteZone(int row)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(row);
    zoneListModel()->deleteZone(zoneRow.zoneId, row);
}

void ZonesWindow::updateSelectedZone(bool enabled)
{
    updateZone(zoneListCurrentIndex(), enabled);
}

void ZonesWindow::deleteSelectedZone()
{
    deleteZone(zoneListCurrentIndex());
}

int ZonesWindow::zoneListCurrentIndex() const
{
    return m_zoneListView->currentRow();
}
