#include "plaintextedit.h"

PlainTextEdit::PlainTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
}

void PlainTextEdit::setText(const QString &text)
{
    if (text.isEmpty() && document()->isEmpty())
        return;  // Workaround to show placeholder text on startup

    setPlainText(text);
}
