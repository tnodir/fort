#include "zoneswindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/tableview.h>
#include <form/dialog/dialogutil.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <model/zonesourcewrapper.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "zonescontroller.h"

namespace {

constexpr int ZONES_HEADER_VERSION = 3;

QLabel *formLabelForField(QFormLayout *formLayout, QWidget *field)
{
    auto label = qobject_cast<QLabel *>(formLayout->labelForField(field));
    Q_ASSERT(label);

    label->setMinimumWidth(100);

    return label;
}

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
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/ip_class.png"));

    // Size
    this->setMinimumSize(500, 400);
}

void ZonesWindow::setupZoneEditForm()
{
    m_formZoneEdit = new QDialog(this);
    m_formZoneEdit->setWindowModality(Qt::WindowModal);
    m_formZoneEdit->setSizeGripEnabled(true);
    m_formZoneEdit->setMinimumWidth(500);

    // Name & Sources
    auto nameLayout = setupZoneEditNameLayout();

    // URL
    setupZoneEditUrlFrame();

    // Text Inline
    setupZoneEditTextFrame();

    // OK/Cancel
    auto buttonsLayout = setupZoneEditButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_frameUrl, 1);
    layout->addWidget(m_frameText);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(buttonsLayout);

    m_formZoneEdit->setLayout(layout);
}

QLayout *ZonesWindow::setupZoneEditNameLayout()
{
    auto formLayout = new QFormLayout();

    // Zone Name
    m_editZoneName = new QLineEdit();
    m_editZoneName->setMaxLength(256);

    formLayout->addRow("Zone Name:", m_editZoneName);
    m_labelZoneName = formLabelForField(formLayout, m_editZoneName);

    // Sources
    setupZoneEditSources();

    formLayout->addRow("Source:", m_comboSources);
    m_labelSource = formLabelForField(formLayout, m_comboSources);

    // Enabled
    m_cbEnabled = new QCheckBox();

    formLayout->addRow(QString(), m_cbEnabled);

    return formLayout;
}

void ZonesWindow::setupZoneEditSources()
{
    m_comboSources = ControlUtil::createComboBox(QStringList(), [&](int /*index*/) {
        const auto zoneSource = ZoneSourceWrapper(m_comboSources->currentData());

        updateZoneEditFormBySource(zoneSource);
    });

    m_comboSources->clear();
    for (const auto &sourceVar : zoneListModel()->zoneSources()) {
        const ZoneSourceWrapper zoneSource(sourceVar);
        m_comboSources->addItem(zoneSource.title(), sourceVar);
    }
    m_comboSources->setCurrentIndex(0);
}

void ZonesWindow::setupZoneEditUrlFrame()
{
    m_frameUrl = new QFrame();

    auto layout = setupZoneEditUrlLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_frameUrl->setLayout(layout);
}

QLayout *ZonesWindow::setupZoneEditUrlLayout()
{
    auto formLayout = new QFormLayout();

    // Custom URL
    m_cbCustomUrl = new QCheckBox();

    formLayout->addRow(QString(), m_cbCustomUrl);

    // URL
    m_editUrl = new QLineEdit();
    m_editUrl->setEnabled(false);
    m_editUrl->setMaxLength(1024);

    formLayout->addRow("URL:", m_editUrl);
    m_labelUrl = formLabelForField(formLayout, m_editUrl);

    // Form Data
    m_editFormData = new QLineEdit();
    m_editFormData->setEnabled(false);

    formLayout->addRow("Form Data:", m_editFormData);
    m_labelFormData = formLabelForField(formLayout, m_editFormData);

    connect(m_cbCustomUrl, &QCheckBox::toggled, this, [&](bool checked) {
        m_editUrl->setEnabled(checked);
        m_editFormData->setEnabled(checked);
    });

    return formLayout;
}

void ZonesWindow::setupZoneEditTextFrame()
{
    m_frameText = new QFrame();

    auto layout = setupZoneEditTextLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_frameText->setLayout(layout);
}

QLayout *ZonesWindow::setupZoneEditTextLayout()
{
    m_editText = new PlainTextEdit();
    m_editText->setPlaceholderText(NetUtil::localIpNetworksText());

    auto layout = new QVBoxLayout();
    layout->addWidget(m_editText);

    return layout;
}

QLayout *ZonesWindow::setupZoneEditButtons()
{
    m_btEditOk = new QPushButton();
    m_btEditOk->setDefault(true);

    m_btEditCancel = new QPushButton();

    connect(m_btEditOk, &QAbstractButton::clicked, this, [&] {
        if (saveZoneEditForm()) {
            m_formZoneEdit->close();
        }
    });
    connect(m_btEditCancel, &QAbstractButton::clicked, m_formZoneEdit, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btEditOk, 1, Qt::AlignRight);
    layout->addWidget(m_btEditCancel);

    return layout;
}

QLayout *ZonesWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddZone = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddZone->setShortcut(Qt::Key_Plus);

    m_actEditZone = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditZone->setShortcut(Qt::Key_Enter);

    m_actRemoveZone = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveZone->setShortcut(Qt::Key_Delete);

    connect(m_actAddZone, &QAction::triggered, this, [&] { updateZoneEditForm(/*isNew=*/true); });
    connect(m_actEditZone, &QAction::triggered, this, [&] { updateZoneEditForm(); });
    connect(m_actRemoveZone, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                [&] { deleteSelectedZone(); }, tr("Are you sure to remove selected zone?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Save As Text
    m_btSaveAsText = ControlUtil::createButton(":/icons/save_as.png", [&] {
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

void ZonesWindow::updateZoneEditForm(bool isNew)
{
    ZoneRow zoneRow;
    if (isNew) {
        zoneRow.sourceCode = ZoneSourceWrapper::textSourceCode();
    } else {
        const int zoneIndex = zoneListCurrentIndex();
        if (zoneIndex < 0)
            return;

        zoneRow = zoneListModel()->zoneRowAt(zoneIndex);
    }

    const ZoneSourceWrapper zoneSource(zoneListModel()->zoneSourceByCode(zoneRow.sourceCode));

    m_formZoneIsNew = isNew;

    m_editZoneName->setText(zoneRow.zoneName);
    m_editZoneName->selectAll();
    m_editZoneName->setFocus();
    m_comboSources->setCurrentIndex(zoneSource.index());
    m_cbEnabled->setChecked(zoneRow.enabled);

    m_cbCustomUrl->setChecked(zoneRow.customUrl);
    m_editUrl->setText(zoneRow.url);
    m_editFormData->setText(zoneRow.formData);
    m_editText->setText(zoneRow.textInline);

    updateZoneEditFormBySource(zoneSource);

    m_formZoneEdit->show();
}

void ZonesWindow::updateZoneEditFormBySource(const ZoneSourceWrapper &zoneSource)
{
    if (!m_cbCustomUrl->isChecked()) {
        m_editUrl->setText(zoneSource.url());
        m_editFormData->setText(zoneSource.formData());
    }

    const bool isTextInline = zoneSource.isTextInline();
    m_frameUrl->setVisible(!isTextInline);
    m_frameText->setVisible(isTextInline);
}

bool ZonesWindow::saveZoneEditForm()
{
    const auto zoneSource = ZoneSourceWrapper(m_comboSources->currentData());

    if (zoneSource.url().isEmpty()) {
        m_cbCustomUrl->setChecked(!zoneSource.isTextInline());
    }

    Zone zone;
    zone.zoneName = m_editZoneName->text();
    zone.sourceCode = zoneSource.code();
    zone.enabled = m_cbEnabled->isChecked();
    zone.customUrl = m_cbCustomUrl->isChecked();
    zone.url = m_editUrl->text();
    zone.formData = m_editFormData->text();
    zone.textInline = m_editText->toPlainText();

    // Validate
    if (!saveZoneEditFormValidate(zone, zoneSource))
        return false;

    // Add new zone
    if (m_formZoneIsNew) {
        return saveZoneEditFormNew(zone);
    }

    // Edit selected zone
    return saveZoneEditFormEdit(zone);
}

bool ZonesWindow::saveZoneEditFormValidate(const Zone &zone, const ZoneSourceWrapper &zoneSource)
{
    // Check zone name
    if (zone.zoneName.isEmpty()) {
        m_editZoneName->setFocus();
        return false;
    }

    // Check custom url
    if (zone.customUrl && zone.url.isEmpty()) {
        m_editUrl->setText(zoneSource.url());
        m_editUrl->selectAll();
        m_editUrl->setFocus();
        m_editFormData->setText(zoneSource.formData());
        return false;
    }

    return true;
}

bool ZonesWindow::saveZoneEditFormNew(Zone &zone)
{
    if (confManager()->addZone(zone)) {
        m_zoneListView->selectCell(zone.zoneId - 1);
        return true;
    }
    return false;
}

bool ZonesWindow::saveZoneEditFormEdit(Zone &zone)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(zoneListCurrentIndex());
    if (zoneRow.isNull())
        return false;

    const bool zoneNameEdited = (zone.zoneName != zoneRow.zoneName);
    const bool zoneEdited = (zone.enabled != zoneRow.enabled || zone.customUrl != zoneRow.customUrl
            || zone.sourceCode != zoneRow.sourceCode || zone.url != zoneRow.url
            || zone.formData != zoneRow.formData || zone.textInline != zoneRow.textInline);

    if (!zoneEdited) {
        if (zoneNameEdited) {
            return confManager()->updateZoneName(zoneRow.zoneId, zone.zoneName);
        }
        return true;
    }

    zone.zoneId = zoneRow.zoneId;

    return confManager()->updateZone(zone);
}

void ZonesWindow::updateZone(int row, bool enabled)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(row);
    if (zoneRow.isNull())
        return;

    Zone zone = zoneRow;
    zone.enabled = enabled;

    confManager()->updateZone(zone);
}

void ZonesWindow::deleteZone(int row)
{
    const auto zoneRow = zoneListModel()->zoneRowAt(row);
    if (zoneRow.isNull())
        return;

    confManager()->deleteZone(zoneRow.zoneId);
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
