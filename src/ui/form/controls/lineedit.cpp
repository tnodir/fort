#include "lineedit.h"

#include <QEvent>

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent) { }

void LineEdit::setStartText(const QString &v)
{
    setText(v);
    setCursorPosition(0);
}

bool LineEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip: {
        setToolTip(text());
    } break;
    }

    return QLineEdit::event(event);
}
