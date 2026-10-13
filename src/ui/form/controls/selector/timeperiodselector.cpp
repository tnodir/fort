#include "timeperiodselector.h"

#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/timeperiodlistmodel.h>

using namespace Fort;

TimePeriodSelector::TimePeriodSelector(QWidget *parent) : QWidget(parent)
{
    setupUi();
    setupController();
}

bool TimePeriodSelector::periodEnabled() const
{
    return m_cbPeriodEnabled->isChecked();
}

void TimePeriodSelector::setPeriodEnabled(bool v)
{
    m_cbPeriodEnabled->setChecked(v);
}

quint8 TimePeriodSelector::periodId() const
{
    return m_comboPeriod->currentData().toUInt();
}

void TimePeriodSelector::setPeriodId(quint8 periodId)
{
    const int index = m_comboPeriod->findData(int(periodId));

    // The deleted Time Period is "No Period"
    m_comboPeriod->setCurrentIndex(index < 0 ? 0 : index);
}

void TimePeriodSelector::retranslateUi()
{
    m_cbPeriodEnabled->setText(tr("Time Period:"));

    m_comboPeriod->setItemText(0, tr("No Period"));

    m_btTimePeriods->setToolTip(tr("Time Periods"));
}

void TimePeriodSelector::setupUi()
{
    m_cbPeriodEnabled = new QCheckBox();

    m_comboPeriod = ControlUtil::createComboBox();
    m_comboPeriod->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboPeriod->setMinimumWidth(200);

    // Time Periods
    m_btTimePeriods = ControlUtil::createIconToolButton(
            ":/icons/clock.png", [&] { windowManager()->showTimePeriodsWindow(); });

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbPeriodEnabled, m_comboPeriod, m_btTimePeriods }, /*margin=*/0);
    layout->setSpacing(0);

    this->setLayout(layout);
}

void TimePeriodSelector::setupController()
{
    updateComboPeriods();

    auto timePeriodListModel = Fort::timePeriodListModel();

    connect(timePeriodListModel, &TimePeriodListModel::modelReset, this,
            &TimePeriodSelector::updateComboPeriods);
    connect(timePeriodListModel, &TimePeriodListModel::dataChanged, this,
            [&](const QModelIndex &topLeft) {
                if (topLeft.column() == int(TimePeriodListColumn::Name)) {
                    updateComboPeriods();
                }
            });
}

void TimePeriodSelector::updateComboPeriods()
{
    const quint8 periodId = this->periodId();

    m_comboPeriod->clear();
    m_comboPeriod->addItem(QString(), 0);

    const auto timePeriodListModel = Fort::timePeriodListModel();

    const int timePeriodsCount = timePeriodListModel->rowCount();
    for (int row = 0; row < timePeriodsCount; ++row) {
        const auto &timePeriodRow = timePeriodListModel->timePeriodRowAt(row);

        m_comboPeriod->addItem(timePeriodRow.name, int(timePeriodRow.periodId));
        m_comboPeriod->setItemData(row + 1, timePeriodRow.intervalsText(), Qt::ToolTipRole);
    }

    retranslateUi();

    setPeriodId(periodId);
}
