#ifndef WEEKDAYSSELECTOR_H
#define WEEKDAYSSELECTOR_H

#include "pushbutton.h"

QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QCheckBox)

class WeekDaysSelector : public PushButton
{
    Q_OBJECT

public:
    explicit WeekDaysSelector(QWidget *parent = nullptr);

    quint8 weekDays() const { return m_weekDays; } // Monday = 1 << 0, ..., Sunday = 1 << 6
    void setWeekDays(quint8 weekDays);

    void retranslateUi();

signals:
    void weekDaysChanged();

private:
    void retranslateWeekDaysText();

    void setupUi();
    QBoxLayout *setupMenuLayout();

    void updateMenu();
    void updateAllWeekDays();

    void onWeekDayClicked();
    void onAllWeekDaysClicked();

    QCheckBox *weekDayCheckBox(int dayOfWeek) const { return m_cbWeekDays[dayOfWeek - 1]; }

private:
    quint8 m_weekDays = 0;

    QCheckBox *m_cbAllWeekDays = nullptr;
    QCheckBox *m_cbWeekDays[7] = {};
};

#endif // WEEKDAYSSELECTOR_H
