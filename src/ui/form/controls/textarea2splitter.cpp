#include "textarea2splitter.h"

#include "textarea2splitterhandle.h"

TextArea2Splitter::TextArea2Splitter(QWidget *parent) : QSplitter(parent)
{
    setupUi();
}

void TextArea2Splitter::setupUi()
{
    setHandleWidth(32);
}

TextArea2SplitterHandle *TextArea2Splitter::handle() const
{
    return static_cast<TextArea2SplitterHandle *>(QSplitter::handle(1));
}

QSplitterHandle *TextArea2Splitter::createHandle()
{
    auto handle = new TextArea2SplitterHandle(orientation(), this);
    return handle;
}
