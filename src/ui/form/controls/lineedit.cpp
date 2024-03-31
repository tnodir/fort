#include "lineedit.h"

#include <QEvent>

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent) { }

bool LineEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip: {
        setToolTip(text());
    } break;
    }

    return QWidget::event(event);
}
