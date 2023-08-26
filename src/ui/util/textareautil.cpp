#include "textareautil.h"

#include <QPlainTextEdit>

#include "stringutil.h"

void TextAreaUtil::moveCursor(QPlainTextEdit *area, int pos, QTextCursor::MoveMode mode)
{
    QTextCursor c = area->textCursor();
    c.setPosition(pos, mode);
    area->setTextCursor(c);
}

void TextAreaUtil::selectText(QPlainTextEdit *area, int start, int end)
{
    moveCursor(area, start);
    moveCursor(area, end, QTextCursor::KeepAnchor);
}

QString TextAreaUtil::selectedText(QPlainTextEdit *area)
{
    QTextCursor c = area->textCursor();
    return c.selectedText();
}

void TextAreaUtil::removeSelectedText(QPlainTextEdit *area)
{
    QTextCursor c = area->textCursor();
    c.removeSelectedText();
}

void TextAreaUtil::appendText(QPlainTextEdit *area, const QString &text)
{
    if (text.isEmpty())
        return;

    // Append the text to area
    const auto areaOldText = area->toPlainText();
    int areaOldLen = areaOldText.size();

    const int lineEnd = StringUtil::lineEnd(areaOldText, areaOldLen - 1);
    if (lineEnd < 0) {
        area->appendPlainText(text);
        if (areaOldLen != 0) {
            ++areaOldLen; // trailing new line
        }
    } else {
        moveCursor(area, lineEnd + 1);
        area->insertPlainText(text);
    }

    // Select new text
    selectText(area, areaOldLen, areaOldLen + text.size());

    // Activate the area
    area->setFocus();
}

void TextAreaUtil::moveAllLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea)
{
    // Cut the text from srcArea
    const auto text = srcArea->toPlainText();
    srcArea->clear();

    // Paste the text to dstArea
    appendText(dstArea, text);
}

void TextAreaUtil::interchangeAllLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea)
{
    // Cut the text from srcArea
    const auto srcText = srcArea->toPlainText();
    srcArea->clear();

    // Cut the text from dstArea
    const auto dstText = dstArea->toPlainText();
    dstArea->clear();

    // Paste texts
    appendText(srcArea, dstText);
    appendText(dstArea, srcText);
}

void TextAreaUtil::moveSelectedLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea)
{
    const auto srcText = srcArea->toPlainText();
    if (srcText.isEmpty())
        return;

    const int srcTextEnd = srcText.size() - 1;

    auto srcCursor = srcArea->textCursor();

    // Adjust to last line, when cursor at the end
    if (!srcCursor.hasSelection() && srcCursor.atEnd() && !srcCursor.atStart()) {
        srcCursor.setPosition(srcTextEnd - 1);
    }

    const int srcSelStart = qBound(0, srcCursor.selectionStart() - 1, srcTextEnd);
    int srcStart = StringUtil::lineStart(srcText, srcSelStart) + 1;

    const int srcSelEnd = srcCursor.selectionEnd();
    const int srcEnd = StringUtil::lineEnd(srcText, srcSelEnd, srcTextEnd) + 1;

    if (srcStart >= srcEnd) {
        if (--srcStart < 0) // try to select empty line
            return;
    }

    // Cut the text from srcArea
    selectText(srcArea, srcStart, srcEnd);
    const auto text = selectedText(srcArea);
    removeSelectedText(srcArea);

    // Paste the text to dstArea
    appendText(dstArea, text);
}
