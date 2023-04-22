#include "combobox.h"

#include <QWheelEvent>

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void ComboBox::wheelEvent(QWheelEvent *e)
{
    if (!hasFocus()) {
        e->ignore();
        return;
    }

    QComboBox::wheelEvent(e);
}
