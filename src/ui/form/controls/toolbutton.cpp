#include "toolbutton.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionToolButton>

ToolButton::ToolButton(QWidget *parent) : QToolButton(parent) { }

void ToolButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && popupMode() == MenuButtonPopup) {
        checkLeftPress(e);
    }

    QToolButton::mousePressEvent(e);

    m_mousePressed = true;
}

void ToolButton::mouseReleaseEvent(QMouseEvent *e)
{
    QToolButton::mouseReleaseEvent(e);

    if (!m_mousePressed)
        return;

    m_mousePressed = false;

    if (e->button() == Qt::RightButton && e->modifiers() == Qt::NoModifier) {
        onRightClicked();
    }
}

void ToolButton::checkLeftPress(QMouseEvent *e)
{
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    const QRect popupr =
            style()->subControlRect(QStyle::CC_ToolButton, &opt, QStyle::SC_ToolButtonMenu, this);

    if (popupr.isValid() && popupr.contains(e->position().toPoint())) {
        emit aboutToShowMenu();
    }
}

void ToolButton::onRightClicked()
{
    if (menu()) {
        emit aboutToShowMenu();
        showMenu();
    }
}
