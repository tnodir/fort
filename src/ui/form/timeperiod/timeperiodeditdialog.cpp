#include "timeperiodeditdialog.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <manager/windowmanager.h>
#include <util/guiutil.h>

#include "timeperiodintervaledit.h"
#include "timeperiodscontroller.h"

namespace {

TimePeriodInterval defaultInterval()
{
    TimePeriodInterval interval;
    interval.timeFrom = "09:00";
    interval.timeTo = "18:00";
    return interval;
}

}

TimePeriodEditDialog::TimePeriodEditDialog(TimePeriodsController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

void TimePeriodEditDialog::initialize(const TimePeriod &timePeriod)
{
    m_timePeriod = timePeriod;

    m_editName->setStartText(timePeriod.name);
    m_editNotes->setText(timePeriod.notes);
    m_cbEnabled->setChecked(timePeriod.enabled);

    setIntervals(isEmpty() ? TimePeriodIntervals { defaultInterval() } : timePeriod.intervals);

    retranslateUi();

    initializeFocus();
}

void TimePeriodEditDialog::initializeFocus()
{
    m_editName->setFocus();
}

void TimePeriodEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelName->setText(tr("Name:"));
    m_labelNotes->setText(tr("Notes:"));

    m_cbEnabled->setText(tr("Enabled"));
    m_cbEnabled->setToolTip(tr("The disabled Time Period doesn't restrict"
                               " its Groups and Speed Limits."));

    m_labelIntervals->setText(tr("Intervals:"));
    m_labelIntervals->setToolTip(tr("The Time Period is active in ANY of its intervals."));

    for (TimePeriodIntervalEdit *intervalEdit : std::as_const(m_intervalEdits)) {
        intervalEdit->retranslateUi();
    }

    m_btAddInterval->setText(tr("Add Interval"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Time Period"));
}

void TimePeriodEditDialog::setupController()
{
    connect(ctrl(), &TimePeriodsController::retranslateUi, this,
            &TimePeriodEditDialog::retranslateUi);
}

void TimePeriodEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/clock.png"));

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumSize(500, 400);
}

QLayout *TimePeriodEditDialog::setupMainLayout()
{
    // Name
    auto nameLayout = setupNameLayout();

    // Intervals
    auto intervalsLayout = setupIntervalsLayout();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(intervalsLayout, 1);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *TimePeriodEditDialog::setupNameLayout()
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

QLayout *TimePeriodEditDialog::setupIntervalsLayout()
{
    m_labelIntervals = ControlUtil::createLabel();

    // Intervals
    m_intervalsLayout = ControlUtil::createVLayout();
    m_intervalsLayout->addStretch();

    // Add Interval
    m_btAddInterval = ControlUtil::createFlatToolButton(
            ":/icons/add.png", [&] { addInterval(defaultInterval()); });

    auto layout = ControlUtil::createVLayout();
    layout->addWidget(m_labelIntervals);
    layout->addLayout(ControlUtil::createScrollLayout(m_intervalsLayout), 1);
    layout->addWidget(m_btAddInterval, 0, Qt::AlignLeft);

    return layout;
}

QLayout *TimePeriodEditDialog::setupButtons()
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

void TimePeriodEditDialog::setIntervals(const TimePeriodIntervals &intervals)
{
    clearIntervals();

    for (const TimePeriodInterval &interval : intervals) {
        addInterval(interval);
    }
}

TimePeriodIntervals TimePeriodEditDialog::intervals() const
{
    TimePeriodIntervals intervals;

    for (const TimePeriodIntervalEdit *intervalEdit : m_intervalEdits) {
        intervals << intervalEdit->interval();
    }

    return intervals;
}

void TimePeriodEditDialog::addInterval(const TimePeriodInterval &interval)
{
    auto intervalEdit = new TimePeriodIntervalEdit();
    intervalEdit->setInterval(interval);
    intervalEdit->retranslateUi();

    connect(intervalEdit, &TimePeriodIntervalEdit::removeClicked, this,
            [=, this] { removeInterval(intervalEdit); });

    // Keep the stretch at the end
    m_intervalsLayout->insertWidget(m_intervalsLayout->count() - 1, intervalEdit);

    m_intervalEdits.append(intervalEdit);
}

void TimePeriodEditDialog::removeInterval(TimePeriodIntervalEdit *intervalEdit)
{
    m_intervalEdits.removeOne(intervalEdit);

    intervalEdit->deleteLater();
}

void TimePeriodEditDialog::clearIntervals()
{
    for (TimePeriodIntervalEdit *intervalEdit : std::as_const(m_intervalEdits)) {
        intervalEdit->deleteLater();
    }

    m_intervalEdits.clear();
}

bool TimePeriodEditDialog::save()
{
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    TimePeriod timePeriod;
    fillTimePeriod(timePeriod);

    // Add new time period
    if (isEmpty()) {
        return ctrl()->addOrUpdateTimePeriod(timePeriod);
    }

    // Edit selected time period
    return saveTimePeriod(timePeriod);
}

bool TimePeriodEditDialog::saveTimePeriod(TimePeriod &timePeriod)
{
    if (!timePeriod.isOptionsEqual(m_timePeriod)) {
        timePeriod.periodId = m_timePeriod.periodId;

        return ctrl()->addOrUpdateTimePeriod(timePeriod);
    }

    if (!timePeriod.isNameEqual(m_timePeriod)) {
        return ctrl()->updateTimePeriodName(m_timePeriod.periodId, timePeriod.name);
    }

    return true;
}

void TimePeriodEditDialog::fillTimePeriod(TimePeriod &timePeriod) const
{
    timePeriod.name = m_editName->text();
    timePeriod.notes = m_editNotes->toPlainText();
    timePeriod.enabled = m_cbEnabled->isChecked();
    timePeriod.intervals = intervals();
}
