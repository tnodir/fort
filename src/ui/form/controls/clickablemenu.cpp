#include "clickablemenu.h"

#include <QMouseEvent>

ClickableMenu::ClickableMenu(QWidget *parent) : QMenu(parent)
{
    setAttribute(Qt::WA_WindowPropagation); // to inherit default font
}

void ClickableMenu::mousePressEvent(QMouseEvent *event)
{
    m_pressed = true;

    QMenu::mousePressEvent(event);
}

void ClickableMenu::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_pressed)
        return;

    m_pressed = false;

    QMenu::mouseReleaseEvent(event);

    if (isVisible()) {
        emit clicked();
    }
}
