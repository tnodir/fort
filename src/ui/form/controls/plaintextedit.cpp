#include "plaintextedit.h"

#include <QContextMenuEvent>
#include <QMenu>

PlainTextEdit::PlainTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setTabChangesFocus(true);
}

bool PlainTextEdit::isEmpty() const
{
    return document()->isEmpty();
}

void PlainTextEdit::setText(const QString &text)
{
    if (text.isEmpty() && isEmpty())
        return; // Workaround to show placeholder text on startup

    setPlainText(text);
}

void PlainTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    if (m_contextActions.isEmpty()) {
        QPlainTextEdit::contextMenuEvent(e);
        return;
    }

    QMenu *menu = createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addActions(m_contextActions);

    menu->popup(e->globalPos());
}
