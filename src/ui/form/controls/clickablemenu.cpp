#include "clickablemenu.h"

#include <QMouseEvent>

ClickableMenu::ClickableMenu(QWidget *parent) : QMenu(parent)
{
    setAttribute(Qt::WA_WindowPropagation); // to inherit default font
}

void ClickableMenu::mousePressEvent(QMouseEvent *event)
{
    QMenu::mousePressEvent(event);

    emit clicked();
}
