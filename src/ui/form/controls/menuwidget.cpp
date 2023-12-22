#include "menuwidget.h"

#include <QEvent>

MenuWidget::MenuWidget(QWidget *parent) : QWidget(parent) { }

bool MenuWidget::event(QEvent *event)
{
    const QEvent::Type type = event->type();
    const bool res = QWidget::event(event);

    switch (type) {
    case QEvent::LayoutRequest: {
        emit layoutRequested();
    } break;
    default:
        break;
    }

    return res;
}
