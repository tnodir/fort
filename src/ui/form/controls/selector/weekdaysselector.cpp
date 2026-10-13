#include "weekdaysselector.h"

#include <QCheckBox>
#include <QFrame>
#include <QLocale>
#include <QMenu>
#include <QVBoxLayout>

#include <conf/timeperiod.h>
#include <form/controls/controlutil.h>

WeekDaysSelector::WeekDaysSelector(QWidget *parent) : PushButton(parent)
{
    setupUi();
}

void WeekDaysSelector::setWeekDays(quint8 weekDays)
{
    if (m_weekDays == weekDays)
        return;

    m_weekDays = weekDays;

    updateMenu();
    retranslateWeekDaysText();
}

void WeekDaysSelector::retranslateUi()
{
    const QLocale locale;

    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        weekDayCheckBox(day)->setText(locale.dayName(day, QLocale::LongFormat));
    }

    m_cbAllWeekDays->setText(tr("All"));

    retranslateWeekDaysText();

    this->setToolTip(tr("Week Days"));
}

void WeekDaysSelector::retranslateWeekDaysText()
{
    TimePeriodInterval interval;
    interval.weekDays = m_weekDays;

    const QString text = interval.weekDaysText();

    this->setText(text.isEmpty() ? tr("No Days") : text);
}

void WeekDaysSelector::setupUi()
{
    this->setMinimumWidth(110);

    auto menu = ControlUtil::createMenuByLayout(setupMenuLayout(), this);
    this->setMenu(menu);

    updateMenu();
}

QBoxLayout *WeekDaysSelector::setupMenuLayout()
{
    auto layout = new QVBoxLayout();

    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        auto c = new QCheckBox();

        connect(c, &QCheckBox::clicked, this, &WeekDaysSelector::onWeekDayClicked);

        m_cbWeekDays[day - 1] = c;

        layout->addWidget(c);
    }

    // All Week Days: checks all of them, unless they're already checked
    m_cbAllWeekDays = new QCheckBox();

    connect(m_cbAllWeekDays, &QCheckBox::clicked, this, &WeekDaysSelector::onAllWeekDaysClicked);

    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_cbAllWeekDays);

    return layout;
}

void WeekDaysSelector::updateMenu()
{
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        weekDayCheckBox(day)->setChecked((m_weekDays & (1u << (day - 1))) != 0);
    }

    updateAllWeekDays();
}

void WeekDaysSelector::updateAllWeekDays()
{
    Qt::CheckState state = Qt::PartiallyChecked;

    if (m_weekDays == 0) {
        state = Qt::Unchecked;
    } else if (m_weekDays == TimePeriodAllWeekDays) {
        state = Qt::Checked;
    }

    m_cbAllWeekDays->setCheckState(state);
}

void WeekDaysSelector::onWeekDayClicked()
{
    quint8 weekDays = 0;

    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        if (weekDayCheckBox(day)->isChecked()) {
            weekDays |= (1u << (day - 1));
        }
    }

    setWeekDays(weekDays);

    emit weekDaysChanged();
}

void WeekDaysSelector::onAllWeekDaysClicked()
{
    const bool allChecked = (m_weekDays == TimePeriodAllWeekDays);

    setWeekDays(allChecked ? 0 : TimePeriodAllWeekDays);

    emit weekDaysChanged();
}
