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
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/zonelistmodel.h"
#include "../../model/zonesourcewrapper.h"
#include "../../util/guiutil.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "../controls/widebutton.h"
#include "zonescontroller.h"

namespace {

#define ZONES_HEADER_VERSION 1

}

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
    auto header = m_zoneListView->horizontalHeader();
    settings()->setZonesHeader(header->saveState());
    settings()->setZonesHeaderVersion(ZONES_HEADER_VERSION);
}

void ZonesWindow::onRestoreWindowState()
{
    if (settings()->zonesHeaderVersion() != ZONES_HEADER_VERSION)
        return;

    auto header = m_zoneListView->horizontalHeader();
    header->restoreState(settings()->zonesHeader());
}

void ZonesWindow::onRetranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddZone->setText(tr("Add"));
    m_actEditZone->setText(tr("Edit"));
    m_actRemoveZone->setText(tr("Remove"));

    m_labelZoneName->setText(tr("Zone Name:"));
    m_labelSource->setText(tr("Source:"));
    m_cbEnabled->setText(tr("Enabled"));
    m_cbStoreText->setText(tr("Store Text"));
    m_cbCustomUrl->setText(tr("Custom URL"));
    m_labelUrl->setText(tr("URL:"));
    m_labelFormData->setText(tr("Form Data:"));
    m_btEditOk->setText(tr("OK"));
    m_btEditCancel->setText(tr("Cancel"));

    m_formZoneEdit->setWindowTitle(tr("Edit Zone"));

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

void ZonesWindow::setupZoneEditForm()
{
    auto formLayout = new QFormLayout();

    // Zone Name
    m_editZoneName = new QLineEdit();

    formLayout->addRow("Zone Name:", m_editZoneName);
    m_labelZoneName = qobject_cast<QLabel *>(formLayout->labelForField(m_editZoneName));

    // Sources
    setupComboSources();

    formLayout->addRow("Source:", m_comboSources);
    m_labelSource = qobject_cast<QLabel *>(formLayout->labelForField(m_comboSources));

    // Enabled
    m_cbEnabled = new QCheckBox();

    formLayout->addRow(QString(), m_cbEnabled);

    // Store Text
    m_cbStoreText = new QCheckBox();

    formLayout->addRow(QString(), m_cbStoreText);

    // Custom URL
    m_cbCustomUrl = new QCheckBox();

    formLayout->addRow(QString(), m_cbCustomUrl);

    // URL
    m_editUrl = new QLineEdit();
    m_editUrl->setEnabled(false);

    formLayout->addRow("URL:", m_editUrl);
    m_labelUrl = qobject_cast<QLabel *>(formLayout->labelForField(m_editUrl));

    // Form Data
    m_editFormData = new QLineEdit();
    m_editFormData->setEnabled(false);

    formLayout->addRow("Form Data:", m_editFormData);
    m_labelFormData = qobject_cast<QLabel *>(formLayout->labelForField(m_editFormData));

    // OK/Cancel
    auto buttonsLayout = new QHBoxLayout();

    m_btEditOk = new QPushButton(QIcon(":/images/tick.png"), QString());
    m_btEditOk->setDefault(true);

    m_btEditCancel = new QPushButton(QIcon(":/images/cancel.png"), QString());

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

    connect(m_cbCustomUrl, &QCheckBox::toggled, [&](bool checked) {
        m_editUrl->setEnabled(checked);
        m_editFormData->setEnabled(checked);
    });

    connect(m_btEditOk, &QAbstractButton::clicked, [&] {
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
        if (onlyFlags) return;

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

    m_actAddZone = editMenu->addAction(QIcon(":/images/map_add.png"), QString());
    m_actAddZone->setShortcut(Qt::Key_Plus);

    m_actEditZone = editMenu->addAction(QIcon(":/images/map_edit.png"), QString());
    m_actEditZone->setShortcut(Qt::Key_Enter);

    m_actRemoveZone = editMenu->addAction(QIcon(":/images/map_delete.png"), QString());
    m_actRemoveZone->setShortcut(Qt::Key_Delete);

    connect(m_actAddZone, &QAction::triggered, [&] {
        updateZoneEditForm(false);
    });
    connect(m_actEditZone, &QAction::triggered, [&] {
        updateZoneEditForm(true);
    });
    connect(m_actRemoveZone, &QAction::triggered, [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove selected zone?"))) {
            deleteSelectedZone();
        }
    });

    m_btEdit = new WideButton(QIcon(":/images/map_edit.png"));
    m_btEdit->setMenu(editMenu);

    layout->addWidget(m_btEdit);
    layout->addStretch();

    return layout;
}

void ZonesWindow::setupTableZones()
{
    m_zoneListView = new TableView();
    m_zoneListView->setIconSize(QSize(24, 24));
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
    };

    refreshTableZonesChanged();

    connect(m_zoneListView, &TableView::currentIndexChanged, this, refreshTableZonesChanged);
}

void ZonesWindow::updateZoneEditForm(bool editCurrentZone)
{
    ZoneRow zoneRow;
    if (editCurrentZone) {
        const auto zoneIndex = zoneListCurrentIndex();
        if (zoneIndex < 0) return;

        zoneRow = zoneListModel()->zoneRowAt(zoneIndex);
    }

    const auto zoneSource = ZoneSourceWrapper(
                zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));

    m_formZoneIsNew = !editCurrentZone;

    m_editZoneName->setText(zoneRow.zoneName);
    m_editZoneName->selectAll();
    m_editZoneName->setFocus();
    m_comboSources->setCurrentIndex(zoneSource.index());
    m_cbEnabled->setChecked(zoneRow.enabled);
    m_cbStoreText->setChecked(zoneRow.storeText);
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
    const bool storeText = m_cbStoreText->isChecked();
    const bool customUrl = m_cbCustomUrl->isChecked();
    const auto url = m_editUrl->text();
    const auto formData = m_editFormData->text();

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
        return zoneListModel()->addZone(zoneName, sourceCode, url, formData,
                                        enabled, storeText, customUrl);
    }

    // Edit selected zone
    const int zoneIndex = zoneListCurrentIndex();
    const auto zoneRow = zoneListModel()->zoneRowAt(zoneIndex);

    const bool zoneNameEdited = (zoneName != zoneRow.zoneName);
    const bool zoneEdited = (sourceCode != zoneRow.sourceCode
            || enabled != zoneRow.enabled
            || storeText != zoneRow.storeText
            || customUrl != zoneRow.customUrl
            || url != zoneRow.url
            || formData != zoneRow.formData);

    if (!zoneEdited) {
        if (zoneNameEdited) {
            return zoneListModel()->updateZoneName(zoneRow.zoneId, zoneName);
        }
        return true;
    }

    return zoneListModel()->updateZone(zoneRow.zoneId, zoneName, sourceCode,
                                       url, formData, enabled, storeText,
                                       customUrl, zoneEdited);
}

void ZonesWindow::updateZone(int row, bool enabled)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(row);

    zoneListModel()->updateZone(zoneRow.zoneId, zoneRow.zoneName,
                                zoneRow.sourceCode, zoneRow.url, zoneRow.formData,
                                enabled, zoneRow.storeText, zoneRow.customUrl);
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
    return m_zoneListView->currentIndex().row();
}

FortManager *ZonesWindow::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *ZonesWindow::settings() const
{
    return ctrl()->settings();
}

ConfManager *ZonesWindow::confManager() const
{
    return ctrl()->confManager();
}

ZoneListModel *ZonesWindow::zoneListModel() const
{
    return fortManager()->zoneListModel();
}
