#include "speedlimiteditdialog.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/doublespinbox.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/spincombo.h>
#include <form/controls/timeperiodselector.h>
#include <manager/windowmanager.h>
#include <util/formatutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

#include "speedlimitscontroller.h"

namespace {

// The first value is for the "Custom" item
inline constexpr std::array speedLimitValues = { 10, 20, 30, 50, 75, 100, 150, 200, 300, 500, 900,
    1024, qRound(1.5 * 1024), 2 * 1024, 3 * 1024, 5 * 1024, qRound(7.5 * 1024), 10 * 1024,
    15 * 1024, 20 * 1024, 30 * 1024, 50 * 1024 };

inline constexpr int SPEED_LIMIT_KBPS_MAX = 9999999;
inline constexpr int SPEED_LIMIT_LATENCY_MAX = 30000;
inline constexpr int SPEED_LIMIT_BUFFER_SIZE_MAX = 2 * 1024 * 1024;

}

SpeedLimitEditDialog::SpeedLimitEditDialog(SpeedLimitsController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

void SpeedLimitEditDialog::initialize(const SpeedLimit &speedLimit)
{
    m_speedLimit = speedLimit;

    retranslateUi();

    m_editName->setStartText(speedLimit.name);
    m_editNotes->setText(speedLimit.notes);
    m_cbEnabled->setChecked(speedLimit.enabled);
    m_rbDownload->setChecked(speedLimit.inbound);
    m_rbUpload->setChecked(!speedLimit.inbound);

    // The Programs refer to the Speed Limit by direction
    m_rbDownload->setEnabled(isEmpty());
    m_rbUpload->setEnabled(isEmpty());

    m_scSpeed->spinBox()->setValue(int(speedLimit.kbps));
    m_spinLatency->setValue(int(speedLimit.latency));
    m_spinPacketLoss->setValue(double(speedLimit.packetLoss) / 100.0);
    m_spinBufferSize->setValue(int(speedLimit.bufferSize));

    m_periodSelector->setPeriodEnabled(speedLimit.periodEnabled);
    m_periodSelector->setPeriodId(speedLimit.periodId);

    initializeFocus();
}

void SpeedLimitEditDialog::initializeFocus()
{
    m_editName->setFocus();
}

void SpeedLimitEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelName->setText(tr("Name:"));
    m_labelNotes->setText(tr("Notes:"));

    m_cbEnabled->setText(tr("Enabled"));

    m_rbDownload->setText(tr("Download"));
    m_rbUpload->setText(tr("Upload"));

    m_labelSpeed->setText(tr("Speed:"));
    retranslateSpeedNames();
    m_labelLatency->setText(tr("Latency:"));
    m_labelPacketLoss->setText(tr("Packet Loss:"));
    m_labelBufferSize->setText(tr("Buffer Size:"));

    m_periodSelector->retranslateUi();
    m_periodSelector->setToolTip(tr("The Speed Limit is active only in this Time Period."));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Speed Limit"));
}

void SpeedLimitEditDialog::retranslateSpeedNames()
{
    QStringList list = { tr("Custom") };

    const auto values = m_scSpeed->values().mid(1);

    for (const int kbps : values) {
        list.append(FormatUtil::formatSpeed(kbps * 1024LL));
    }

    m_scSpeed->setNames(list);
}

void SpeedLimitEditDialog::setupController()
{
    connect(ctrl(), &SpeedLimitsController::retranslateUi, this,
            &SpeedLimitEditDialog::retranslateUi);
}

void SpeedLimitEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/speedometer.png"));

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(500);
}

QLayout *SpeedLimitEditDialog::setupMainLayout()
{
    // Name
    auto nameLayout = setupNameLayout();

    // Download/Upload Direction
    auto directionLayout = setupDirectionLayout();

    // Limit
    auto limitLayout = setupLimitLayout();

    // Time Period
    m_periodSelector = new TimePeriodSelector();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(directionLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(limitLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_periodSelector, 0, Qt::AlignLeft);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *SpeedLimitEditDialog::setupNameLayout()
{
    auto layout = new QFormLayout();

    // Name
    m_editName = new LineEdit();
    m_editName->setMaxLength(256);

    layout->addRow("Name:", m_editName);
    m_labelName = ControlUtil::formRowLabel(layout, m_editName);

    // Notes
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(40);

    layout->addRow("Notes:", m_editNotes);
    m_labelNotes = ControlUtil::formRowLabel(layout, m_editNotes);

    // Enabled
    m_cbEnabled = new QCheckBox();

    layout->addRow(QString(), m_cbEnabled);

    return layout;
}

QLayout *SpeedLimitEditDialog::setupDirectionLayout()
{
    // Download
    m_rbDownload = new QRadioButton();
    m_rbDownload->setIcon(IconCache::icon(":/icons/green_down.png"));
    m_rbDownload->setChecked(true);

    // Upload
    m_rbUpload = new QRadioButton();
    m_rbUpload->setIcon(IconCache::icon(":/icons/blue_up.png"));

    auto layout = ControlUtil::createHLayoutByWidgets(
            { /*stretch*/ nullptr, m_rbDownload, m_rbUpload, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

QLayout *SpeedLimitEditDialog::setupLimitLayout()
{
    auto layout = new QFormLayout();
    layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // Speed
    m_scSpeed = new SpinCombo();
    m_scSpeed->setValues(speedLimitValues);
    m_scSpeed->spinBox()->setRange(1, SPEED_LIMIT_KBPS_MAX); // the zero speed would be ignored
    m_scSpeed->spinBox()->setSuffix(" Kb/s");

    layout->addRow("Speed:", m_scSpeed);
    m_labelSpeed = ControlUtil::formRowLabel(layout, m_scSpeed);

    // Latency
    m_spinLatency = ControlUtil::createSpinBox();
    m_spinLatency->setFixedWidth(110);
    m_spinLatency->setRange(0, SPEED_LIMIT_LATENCY_MAX);
    m_spinLatency->setSuffix(" ms");

    layout->addRow("Latency:", m_spinLatency);
    m_labelLatency = ControlUtil::formRowLabel(layout, m_spinLatency);

    // Packet Loss
    m_spinPacketLoss = new DoubleSpinBox();
    m_spinPacketLoss->setFixedWidth(110);
    m_spinPacketLoss->setRange(0, 100.0);
    m_spinPacketLoss->setDecimals(2);
    m_spinPacketLoss->setSuffix(" %");

    layout->addRow("Packet Loss:", m_spinPacketLoss);
    m_labelPacketLoss = ControlUtil::formRowLabel(layout, m_spinPacketLoss);

    // Buffer Size
    m_spinBufferSize = ControlUtil::createSpinBox();
    m_spinBufferSize->setFixedWidth(110);
    m_spinBufferSize->setRange(0, SPEED_LIMIT_BUFFER_SIZE_MAX);
    m_spinBufferSize->setSuffix(" bytes");

    layout->addRow("Buffer Size:", m_spinBufferSize);
    m_labelBufferSize = ControlUtil::formRowLabel(layout, m_spinBufferSize);

    return layout;
}

QLayout *SpeedLimitEditDialog::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
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

bool SpeedLimitEditDialog::save()
{
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    SpeedLimit speedLimit;
    fillSpeedLimit(speedLimit);

    // Add new speed limit
    if (isEmpty()) {
        return ctrl()->addOrUpdateSpeedLimit(speedLimit);
    }

    // Edit selected speed limit
    return saveSpeedLimit(speedLimit);
}

bool SpeedLimitEditDialog::saveSpeedLimit(SpeedLimit &speedLimit)
{
    if (!speedLimit.isOptionsEqual(m_speedLimit)) {
        speedLimit.limitId = m_speedLimit.limitId;

        return ctrl()->addOrUpdateSpeedLimit(speedLimit);
    }

    if (!speedLimit.isNameEqual(m_speedLimit)) {
        return ctrl()->updateSpeedLimitName(m_speedLimit.limitId, speedLimit.name);
    }

    return true;
}

void SpeedLimitEditDialog::fillSpeedLimit(SpeedLimit &speedLimit) const
{
    speedLimit.name = m_editName->text();
    speedLimit.notes = m_editNotes->toPlainText();
    speedLimit.enabled = m_cbEnabled->isChecked();
    speedLimit.inbound = m_rbDownload->isChecked();
    speedLimit.kbps = quint32(m_scSpeed->spinBox()->value());
    speedLimit.latency = quint32(m_spinLatency->value());
    speedLimit.packetLoss = quint16(qRound(m_spinPacketLoss->value() * 100.0));
    speedLimit.bufferSize = quint32(m_spinBufferSize->value());
    speedLimit.periodEnabled = m_periodSelector->periodEnabled();
    speedLimit.periodId = m_periodSelector->periodId();
}
