#include "timeperiodintervaledit.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimeEdit>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <form/controls/weekdaysselector.h>

namespace {

inline constexpr char timeFormat[] = "HH:mm";

QTimeEdit *createTimeEdit()
{
    auto c = new QTimeEdit();
    c->setFixedWidth(70);
    c->setDisplayFormat(timeFormat);
    c->setWrapping(true);
    return c;
}

QString fromTime(const QTime &time)
{
    return time.toString(timeFormat);
}

QTime toTime(const QString &text)
{
    if (text.isEmpty())
        return QTime(0, 0);

    return QTime::fromString(text, timeFormat);
}

}

TimePeriodIntervalEdit::TimePeriodIntervalEdit(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

TimePeriodInterval TimePeriodIntervalEdit::interval() const
{
    TimePeriodInterval interval;

    interval.weekDays = m_weekDaysSelector->weekDays();
    interval.timeFrom = fromTime(m_timeFrom->time());
    interval.timeTo = fromTime(m_timeTo->time());

    return interval;
}

void TimePeriodIntervalEdit::setInterval(const TimePeriodInterval &interval)
{
    m_weekDaysSelector->setWeekDays(interval.weekDays);
    m_timeFrom->setTime(toTime(interval.timeFrom));
    m_timeTo->setTime(toTime(interval.timeTo));
}

void TimePeriodIntervalEdit::retranslateUi()
{
    m_timeFrom->setToolTip(tr("From"));
    m_timeTo->setToolTip(tr("To (the same time means the whole 24 hours)"));

    m_weekDaysSelector->retranslateUi();

    m_btRemove->setToolTip(tr("Remove"));
}

void TimePeriodIntervalEdit::setupUi()
{
    // Interval
    auto intervalLayout = setupIntervalLayout();

    auto layout = ControlUtil::createVLayout();
    layout->addLayout(intervalLayout);
    layout->addWidget(ControlUtil::createHSeparator());

    this->setLayout(layout);
}

QLayout *TimePeriodIntervalEdit::setupIntervalLayout()
{
    // Time
    m_timeFrom = createTimeEdit();
    m_timeTo = createTimeEdit();

    // Week Days
    m_weekDaysSelector = new WeekDaysSelector();
    m_weekDaysSelector->setWeekDays(TimePeriodAllWeekDays);

    // Remove
    m_btRemove =
            ControlUtil::createIconToolButton(":/icons/delete.png", [&] { emit removeClicked(); });

    auto layout = ControlUtil::createHLayout();
    layout->addWidget(m_timeFrom);
    layout->addWidget(ControlUtil::createLabel(QString(QChar(0x2013)))); // –
    layout->addWidget(m_timeTo);
    layout->addWidget(m_weekDaysSelector);
    layout->addWidget(ControlUtil::createVSeparator());
    layout->addWidget(m_btRemove);
    layout->addStretch();

    return layout;
}
