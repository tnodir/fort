#ifndef SPINBOX_H
#define SPINBOX_H

#include <QSpinBox>

class SpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit SpinBox(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *e) override;
};

#endif // SPINBOX_H
