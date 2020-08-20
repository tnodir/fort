#ifndef TEXTAREAUTIL_H
#define TEXTAREAUTIL_H

#include <QObject>
#include <QTextCursor>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

class TextAreaUtil
{
public:
    static void moveCursor(
            QPlainTextEdit *area, int pos, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    static void selectText(QPlainTextEdit *area, int start, int end);
    static QString selectedText(QPlainTextEdit *area);
    static void removeSelectedText(QPlainTextEdit *area);

    static void appendText(QPlainTextEdit *area, const QString &text);

    static void moveAllLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea);
    static void interchangeAllLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea);

    static void moveSelectedLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea);
};

#endif // TEXTAREAUTIL_H
