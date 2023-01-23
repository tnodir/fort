#include "clickablemenu.h"

#include <QMouseEvent>

ClickableMenu::ClickableMenu(QWidget *parent) : QMenu(parent)
{
    setAttribute(Qt::WA_WindowPropagation); // to inherit default font
}

void ClickableMenu::mouseReleaseEvent(QMouseEvent *event)
{
    QMenu::mouseReleaseEvent(event);

    if (isVisible()) {
        emit clicked();
    }
}
