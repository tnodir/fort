#include "pushbutton.h"

#include <QMouseEvent>

PushButton::PushButton(QWidget *parent) : QPushButton(parent) { }

PushButton::PushButton(const QIcon &icon, const QString &text, QWidget *parent) :
    QPushButton(icon, text, parent)
{
}

void PushButton::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);

    setMousePressed(true);
}

void PushButton::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);

    if (!mousePressed())
        return;

    setMousePressed(false);

    if (e->button() == Qt::RightButton && e->modifiers() == Qt::NoModifier) {
        onRightClicked();
    }
}

void PushButton::onRightClicked()
{
    if (menu()) {
        showMenu();
    }
}
