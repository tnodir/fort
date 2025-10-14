#include "zoneeditdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <model/zonesourcewrapper.h>
#include <util/guiutil.h>
#include <util/net/netutil.h>

#include "zonescontroller.h"

using namespace Fort;

ZoneEditDialog::ZoneEditDialog(ZonesController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

void ZoneEditDialog::initialize(const Zone &zone)
{
    m_zone = zone;

    retranslateUi();

    const ZoneSourceWrapper zoneSource(zoneListModel()->zoneSourceByCode(zone.sourceCode));

    m_editName->setStartText(zone.zoneName);
    m_comboSources->setCurrentIndex(zoneSource.id());
    m_cbEnabled->setChecked(zone.enabled);

    m_cbCustomUrl->setChecked(zone.customUrl);
    m_editUrl->setText(zone.url);
    m_editFormData->setText(zone.formData);
    m_editText->setText(zone.textInline);

    initializeBySource(zoneSource);

    initializeFocus();
}

void ZoneEditDialog::initializeBySource(const ZoneSourceWrapper &zoneSource)
{
    if (!m_cbCustomUrl->isChecked()) {
        m_editUrl->setText(zoneSource.url());
        m_editFormData->setText(zoneSource.formData());
    }

    const bool isTextInline = zoneSource.isTextInline();
    m_frameUrl->setVisible(!isTextInline);
    m_frameText->setVisible(isTextInline);
}

void ZoneEditDialog::initializeFocus()
{
    m_editName->setFocus();
}

void ZoneEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelName->setText(tr("Name:"));
    m_labelSource->setText(tr("Source:"));
    retranslateComboSources();

    m_cbEnabled->setText(tr("Enabled"));
    m_cbCustomUrl->setText(tr("Custom URL"));
    m_labelUrl->setText(tr("URL:"));
    m_labelFormData->setText(tr("Form Data:"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Zone"));
}

void ZoneEditDialog::retranslateComboSources()
{
    const int n = m_comboSources->count();
    for (int i = 0; i < n; ++i) {
        const auto title = zoneListModel()->zoneSourceTitleById(i);

        m_comboSources->setItemText(i, title);
        m_comboSources->setItemData(i, title, Qt::ToolTipRole);
    }
}

void ZoneEditDialog::setupController()
{
    connect(ctrl(), &ZonesController::retranslateUi, this, &ZoneEditDialog::retranslateUi);
}

void ZoneEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/ip_class.png"));

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(500);
}

QLayout *ZoneEditDialog::setupMainLayout()
{
    // Name & Sources
    auto nameLayout = setupNameLayout();

    // URL
    setupUrlFrame();

    // Text Inline
    setupTextFrame();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_frameUrl, 1);
    layout->addWidget(m_frameText);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *ZoneEditDialog::setupNameLayout()
{
    auto layout = new QFormLayout();

    // Name
    m_editName = new LineEdit();
    m_editName->setMaxLength(256);

    layout->addRow("Name:", m_editName);
    m_labelName = ControlUtil::formRowLabel(layout, m_editName);

    // Sources
    setupComboSources();

    layout->addRow("Source:", m_comboSources);
    m_labelSource = ControlUtil::formRowLabel(layout, m_comboSources);

    // Enabled
    m_cbEnabled = new QCheckBox();

    layout->addRow(QString(), m_cbEnabled);

    return layout;
}

void ZoneEditDialog::setupComboSources()
{
    m_comboSources = ControlUtil::createComboBox(QStringList(), [&](int index) {
        const ZoneSourceWrapper zoneSource(zoneListModel()->zoneSourceById(index));

        initializeBySource(zoneSource);
    });

    m_comboSources->clear();
    for (const auto &sourceVar : zoneListModel()->zoneSources()) {
        const ZoneSourceWrapper zoneSource(sourceVar);

        m_comboSources->addItem(zoneSource.title());
    }
    m_comboSources->setCurrentIndex(0);
}

void ZoneEditDialog::setupUrlFrame()
{
    m_frameUrl = new QFrame();

    auto layout = setupUrlLayout();

    m_frameUrl->setLayout(layout);
}

QLayout *ZoneEditDialog::setupUrlLayout()
{
    auto layout = new QFormLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Custom URL
    m_cbCustomUrl = new QCheckBox();

    layout->addRow(QString(), m_cbCustomUrl);

    // URL
    m_editUrl = new LineEdit();
    m_editUrl->setEnabled(false);
    m_editUrl->setMaxLength(1024);

    layout->addRow("URL:", m_editUrl);
    m_labelUrl = ControlUtil::formRowLabel(layout, m_editUrl);

    // Form Data
    m_editFormData = new LineEdit();
    m_editFormData->setEnabled(false);

    layout->addRow("Form Data:", m_editFormData);
    m_labelFormData = ControlUtil::formRowLabel(layout, m_editFormData);

    connect(m_cbCustomUrl, &QCheckBox::toggled, this, [&](bool checked) {
        m_editUrl->setEnabled(checked);
        m_editFormData->setEnabled(checked);
    });

    return layout;
}

void ZoneEditDialog::setupTextFrame()
{
    m_frameText = new QFrame();

    auto layout = setupTextLayout();

    m_frameText->setLayout(layout);
}

QLayout *ZoneEditDialog::setupTextLayout()
{
    m_editText = new PlainTextEdit();
    m_editText->setPlaceholderText(NetUtil::localIpNetworksText());

    auto layout = ControlUtil::createVLayout();
    layout->addWidget(m_editText);

    return layout;
}

QLayout *ZoneEditDialog::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            emit saved();
            this->close();
        }
    });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btCancel);

    return layout;
}

bool ZoneEditDialog::save()
{
    const ZoneSourceWrapper zoneSource(
            zoneListModel()->zoneSourceById(m_comboSources->currentIndex()));

    if (!validateFields(zoneSource))
        return false;

    Zone zone;
    fillZone(zone, zoneSource);

    // Add new zone
    if (isEmpty()) {
        return ctrl()->addOrUpdateZone(zone);
    }

    // Edit selected zone
    return saveZone(zone);
}

bool ZoneEditDialog::saveZone(Zone &zone)
{
    if (!zone.isOptionsEqual(m_zone)) {
        zone.zoneId = m_zone.zoneId;

        return ctrl()->addOrUpdateZone(zone);
    }

    if (!zone.isNameEqual(m_zone)) {
        return ctrl()->updateZoneName(m_zone.zoneId, zone.zoneName);
    }

    return true;
}

bool ZoneEditDialog::validateFields(const ZoneSourceWrapper &zoneSource) const
{
    // Name
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    // Custom URL
    if (m_cbCustomUrl->isChecked() && m_editUrl->text().isEmpty()) {
        m_editUrl->setText(zoneSource.url());
        m_editUrl->selectAll();
        m_editUrl->setFocus();
        m_editFormData->setText(zoneSource.formData());
        return false;
    }

    return true;
}

void ZoneEditDialog::fillZone(Zone &zone, const ZoneSourceWrapper &zoneSource) const
{
    if (zoneSource.url().isEmpty()) {
        m_cbCustomUrl->setChecked(!zoneSource.isTextInline());
    }

    zone.zoneName = m_editName->text();
    zone.sourceCode = zoneSource.code();
    zone.enabled = m_cbEnabled->isChecked();
    zone.customUrl = m_cbCustomUrl->isChecked();
    zone.url = m_editUrl->text();
    zone.formData = m_editFormData->text();
    zone.textInline = m_editText->toPlainText();
}
