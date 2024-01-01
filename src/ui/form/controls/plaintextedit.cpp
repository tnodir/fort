#include "plaintextedit.h"

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
