#ifndef TIMEPERIODSELECTOR_H
#define TIMEPERIODSELECTOR_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class TimePeriodSelector : public QWidget
{
    Q_OBJECT

public:
    explicit TimePeriodSelector(QWidget *parent = nullptr);

    bool periodEnabled() const;
    void setPeriodEnabled(bool v);

    quint8 periodId() const;
    void setPeriodId(quint8 periodId);

    void retranslateUi();

private:
    void setupUi();
    void setupController();

    void updateComboPeriods();

private:
    QCheckBox *m_cbPeriodEnabled = nullptr;
    QComboBox *m_comboPeriod = nullptr;
    QToolButton *m_btTimePeriods = nullptr;
};

#endif // TIMEPERIODSELECTOR_H
