#ifndef LABELDOUBLESPIN_H
#define LABELDOUBLESPIN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

class LabelDoubleSpin : public QWidget
{
    Q_OBJECT

public:
    explicit LabelDoubleSpin(QWidget *parent = nullptr);

    QHBoxLayout *boxLayout() const;
    QLabel *label() const { return m_label; }
    QDoubleSpinBox *spinBox() const { return m_spinBox; }

private:
    void setupUi();
    void setupSpin();

private:
    QLabel *m_label = nullptr;
    QDoubleSpinBox *m_spinBox = nullptr;
};

#endif // LABELDOUBLESPIN_H
