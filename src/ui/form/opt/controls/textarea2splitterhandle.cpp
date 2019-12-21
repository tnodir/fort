#include "textarea2splitterhandle.h"

#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>

#include "../../../fortmanager.h"
#include "../../../util/stringutil.h"
#include "../../controls/controlutil.h"
#include "../optionscontroller.h"
#include "textarea2splitter.h"

namespace {

void moveCursor(QPlainTextEdit *area, int pos,
                QTextCursor::MoveMode mode = QTextCursor::MoveAnchor)
{
    QTextCursor c = area->textCursor();
    c.setPosition(pos, mode);
    area->setTextCursor(c);
}

void selectText(QPlainTextEdit *area, int start, int end)
{
    moveCursor(area, start);
    moveCursor(area, end, QTextCursor::KeepAnchor);
}

QString selectedText(QPlainTextEdit *area)
{
    QTextCursor c = area->textCursor();
    return c.selectedText();
}

void removeSelectedText(QPlainTextEdit *area)
{
    QTextCursor c = area->textCursor();
    c.removeSelectedText();
}

void appendText(QPlainTextEdit *area, const QString &text)
{
    if (text.isEmpty())
        return;

    // Append the text to area
    const auto areaOldText = area->toPlainText();
    const int areaOldLen = areaOldText.size();

    const int lineEnd = StringUtil::lineEnd(areaOldText, areaOldLen - 1);
    if (lineEnd < 0) {
        area->appendPlainText(text);
    } else {
        moveCursor(area, lineEnd + 1);
        area->insertPlainText(text);
    }

    // Select new text
    selectText(area, areaOldLen, areaOldLen + text.size());

    // Activate the area
    area->setFocus();
}

void moveAllLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea)
{
    // Cut the text from srcArea
    const auto text = srcArea->toPlainText();
    srcArea->clear();

    // Paste the text to dstArea
    appendText(dstArea, text);
}

void moveSelectedLines(QPlainTextEdit *srcArea, QPlainTextEdit *dstArea)
{
    const auto srcText = srcArea->toPlainText();
    if (srcText.isEmpty())
        return;

    const int srcTextEnd = srcText.size() - 1;

    auto srcCursor = srcArea->textCursor();

    // Adgust to last line, when cursor at the end
    if (!srcCursor.hasSelection()
            && srcCursor.atEnd()
            && !srcCursor.atStart()) {
        srcCursor.setPosition(srcTextEnd - 1);
    }

    const int srcSelStart = qBound(0, srcCursor.selectionStart() - 1, srcTextEnd);
    int srcStart = StringUtil::lineStart(srcText, srcSelStart) + 1;

    const int srcSelEnd = srcCursor.selectionEnd();
    const int srcEnd = StringUtil::lineEnd(srcText, srcSelEnd, srcTextEnd) + 1;

    if (srcStart >= srcEnd
            && --srcStart < 0)  // try to select empty line
        return;

    // Cut the text from srcArea
    selectText(srcArea, srcStart, srcEnd);
    const auto text = selectedText(srcArea);
    removeSelectedText(srcArea);

    // Paste the text to dstArea
    appendText(dstArea, text);
}

QPushButton *createToolButton(const QString &iconPath,
                              const std::function<void ()> &onClicked)
{
    auto c = ControlUtil::createButton(iconPath, onClicked);
    c->setFixedSize(32, 32);
    c->setFlat(true);
    c->setCursor(Qt::PointingHandCursor);
    c->setFocusPolicy(Qt::NoFocus);
    return c;
}

}

TextArea2SplitterHandle::TextArea2SplitterHandle(bool selectFileEnabled,
                                                 Qt::Orientation o,
                                                 QSplitter *parent) :
    QSplitterHandle(o, parent)
{
    setupUi(selectFileEnabled);
}

TextArea2Splitter *TextArea2SplitterHandle::splitter() const
{
    return qobject_cast<TextArea2Splitter *>(QSplitterHandle::splitter());
}

OptionsController *TextArea2SplitterHandle::ctrl() const
{
    return splitter()->ctrl();
}

FortManager *TextArea2SplitterHandle::fortManager() const
{
    return ctrl()->fortManager();
}

void TextArea2SplitterHandle::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
}

void TextArea2SplitterHandle::mouseMoveEvent(QMouseEvent *e)
{
    // Avoid dragging by buttons
    if (e->button() == Qt::LeftButton) {
        if (e->isAccepted())
            return;
    }

    QSplitterHandle::mouseMoveEvent(e);
}

void TextArea2SplitterHandle::setupUi(bool selectFileEnabled)
{
    const auto direction = (orientation() == Qt::Horizontal)
            ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;

    auto layout = new QBoxLayout(direction);
    layout->setMargin(0);
    layout->setSpacing(10);

    m_btMoveAllFrom1To2 = createToolButton(":/images/control_fastforward.png", [&] {
        moveAllLines(textArea1(), textArea2());
    });
    m_btMoveSelectedFrom1To2 = createToolButton(":/images/control_play.png", [&] {
        moveSelectedLines(textArea1(), textArea2());
    });
    m_btMoveSelectedFrom2To1 = createToolButton(":/images/control_play_backward.png", [&] {
        moveSelectedLines(textArea2(), textArea1());
    });
    m_btMoveAllFrom2To1 = createToolButton(":/images/control_rewind.png", [&] {
        moveAllLines(textArea2(), textArea1());
    });

    layout->addStretch(1);
    layout->addWidget(m_btMoveAllFrom1To2, 0, Qt::AlignHCenter);
    layout->addWidget(m_btMoveSelectedFrom1To2, 0, Qt::AlignHCenter);
    layout->addWidget(m_btMoveSelectedFrom2To1, 0, Qt::AlignHCenter);
    layout->addWidget(m_btMoveAllFrom2To1, 0, Qt::AlignHCenter);

    if (selectFileEnabled) {
        m_btSelectFile = createToolButton(":/images/folder_explore.png", [&] {
            auto area = textArea1()->hasFocus() ? textArea1() : textArea2();
            const auto filePaths = fortManager()->getOpenFileNames(
                        m_btSelectFile->text(),
                        tr("Programs (*.exe);;All files (*.*)"));

            if (!filePaths.isEmpty()) {
                appendText(area, filePaths.join('\n'));
            }
        });

        layout->addWidget(m_btSelectFile, 0, Qt::AlignHCenter);
    }
    layout->addStretch(1);

    this->setLayout(layout);
}
