#include "toolbutton.h"

#include <QMouseEvent>

ToolButton::ToolButton(QWidget *parent) : QToolButton(parent) { }

void ToolButton::mousePressEvent(QMouseEvent *e)
{
    QToolButton::mousePressEvent(e);

    m_mousePressed = true;
}

void ToolButton::mouseReleaseEvent(QMouseEvent *e)
{
    QToolButton::mouseReleaseEvent(e);

    if (!m_mousePressed)
        return;

    m_mousePressed = false;

    if (e->button() == Qt::RightButton) {
        emit rightClicked();
    }
}
