#include "clickablemenu.h"

#include <QMouseEvent>

ClickableMenu::ClickableMenu(QWidget *parent) : QMenu(parent)
{
    setAttribute(Qt::WA_WindowPropagation); // to inherit default font
}

void ClickableMenu::mousePressEvent(QMouseEvent *event)
{
    QMenu::mousePressEvent(event);

    m_pressed = true;
}

void ClickableMenu::mouseReleaseEvent(QMouseEvent *event)
{
    QMenu::mouseReleaseEvent(event);

    if (!m_pressed)
        return;

    m_pressed = false;

    if (isVisible()) {
        emit clicked();
    }
}
