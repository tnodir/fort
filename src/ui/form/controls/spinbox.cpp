#include "spinbox.h"

#include <QWheelEvent>

SpinBox::SpinBox(QWidget *parent) : QSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void SpinBox::wheelEvent(QWheelEvent *e)
{
    if (!hasFocus()) {
        e->ignore();
        return;
    }

    QSpinBox::wheelEvent(e);
}
