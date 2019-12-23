#ifndef CHECKTIMEPERIOD_H
#define CHECKTIMEPERIOD_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QTimeEdit)

class CheckTimePeriod : public QWidget
{
    Q_OBJECT

public:
    explicit CheckTimePeriod(QWidget *parent = nullptr);

    QCheckBox *checkBox() const { return m_checkBox; }
    QTimeEdit *timeEdit1() const { return m_timeEdit1; }
    QTimeEdit *timeEdit2() const { return m_timeEdit2; }

    static QString timeFormat() { return "HH:mm"; }
    static QString fromTime(const QTime &time);
    static QTime toTime(const QString &text);

private:
    void setupUi();

    static QTimeEdit *createTimeEdit();

private:
    QCheckBox *m_checkBox = nullptr;
    QTimeEdit *m_timeEdit1 = nullptr;
    QTimeEdit *m_timeEdit2 = nullptr;
};

#endif // CHECKTIMEPERIOD_H
