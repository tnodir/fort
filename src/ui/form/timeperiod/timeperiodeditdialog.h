#ifndef TIMEPERIODEDITDIALOG_H
#define TIMEPERIODEDITDIALOG_H

#include <QDialog>

#include <conf/timeperiod.h>

QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class LineEdit;
class PlainTextEdit;
class TimePeriodIntervalEdit;
class TimePeriodsController;

class TimePeriodEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimePeriodEditDialog(TimePeriodsController *ctrl, QWidget *parent = nullptr);

    TimePeriodsController *ctrl() const { return m_ctrl; }

    bool isEmpty() const { return m_timePeriod.periodId == 0; }

    void initialize(const TimePeriod &timePeriod);

protected slots:
    void retranslateUi();

private:
    void initializeFocus();

    void setupController();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupNameLayout();
    QLayout *setupIntervalsLayout();
    QLayout *setupButtons();

    void setIntervals(const TimePeriodIntervals &intervals);
    TimePeriodIntervals intervals() const;

    void addInterval(const TimePeriodInterval &interval);
    void removeInterval(TimePeriodIntervalEdit *intervalEdit);
    void clearIntervals();

    bool save();
    bool saveTimePeriod(TimePeriod &timePeriod);

    void fillTimePeriod(TimePeriod &timePeriod) const;

private:
    TimePeriodsController *m_ctrl = nullptr;

    QLabel *m_labelName = nullptr;
    LineEdit *m_editName = nullptr;
    QLabel *m_labelNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QLabel *m_labelIntervals = nullptr;
    QBoxLayout *m_intervalsLayout = nullptr;
    QToolButton *m_btAddInterval = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    QList<TimePeriodIntervalEdit *> m_intervalEdits;

    TimePeriod m_timePeriod;
};

#endif // TIMEPERIODEDITDIALOG_H
