#ifndef TIMEPERIODINTERVALEDIT_H
#define TIMEPERIODINTERVALEDIT_H

#include <QWidget>

#include <conf/timeperiod.h>

QT_FORWARD_DECLARE_CLASS(QTimeEdit)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class WeekDaysSelector;

class TimePeriodIntervalEdit : public QWidget
{
    Q_OBJECT

public:
    explicit TimePeriodIntervalEdit(QWidget *parent = nullptr);

    TimePeriodInterval interval() const;
    void setInterval(const TimePeriodInterval &interval);

    void retranslateUi();

signals:
    void removeClicked();

private:
    void setupUi();
    QLayout *setupIntervalLayout();

private:
    QTimeEdit *m_timeFrom = nullptr;
    QTimeEdit *m_timeTo = nullptr;
    WeekDaysSelector *m_weekDaysSelector = nullptr;
    QToolButton *m_btRemove = nullptr;
};

#endif // TIMEPERIODINTERVALEDIT_H
