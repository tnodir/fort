#include "doublespinbox.h"

#include <QWheelEvent>

DoubleSpinBox::DoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void DoubleSpinBox::wheelEvent(QWheelEvent *e)
{
    if (!hasFocus()) {
        e->ignore();
        return;
    }

    QDoubleSpinBox::wheelEvent(e);
}
