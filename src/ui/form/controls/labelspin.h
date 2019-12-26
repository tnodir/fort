#ifndef LABELSPIN_H
#define LABELSPIN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

class LabelSpin : public QWidget
{
    Q_OBJECT

public:
    explicit LabelSpin(QWidget *parent = nullptr);

    QHBoxLayout *boxLayout() const;
    QLabel *label() const { return m_label; }
    QSpinBox *spinBox() const { return m_spinBox; }

private:
    void setupUi();
    void setupSpin();

private:
    QLabel *m_label = nullptr;
    QSpinBox *m_spinBox = nullptr;
};

#endif // LABELSPIN_H
