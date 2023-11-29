#ifndef DOUBLESPINBOX_H
#define DOUBLESPINBOX_H

#include <QDoubleSpinBox>

class DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit DoubleSpinBox(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *e) override;
};

#endif // DOUBLESPINBOX_H
