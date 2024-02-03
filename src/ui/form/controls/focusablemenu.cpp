#include "focusablemenu.h"

#include <QEvent>
#include <QKeyEvent>

FocusableMenu::FocusableMenu(QWidget *parent) : QMenu(parent)
{
    setAttribute(Qt::WA_WindowPropagation); // to inherit default font
}

bool FocusableMenu::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            return QWidget::event(e);
        }
    } break;
    default:
        break;
    }

    return QMenu::event(e);
}

bool FocusableMenu::focusNextPrevChild(bool next)
{
    return QWidget::focusNextPrevChild(next);
}
