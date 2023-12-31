#include "textarea2splitterhandle.h"

#include <QBoxLayout>
#include <QPainter>
#include <QPlainTextEdit>
#include <QStyleOption>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <util/textareautil.h>

#include "textarea2splitter.h"

TextArea2SplitterHandle::TextArea2SplitterHandle(Qt::Orientation o, QSplitter *parent) :
    QSplitterHandle(o, parent)
{
    setupUi();
}

QPlainTextEdit *TextArea2SplitterHandle::currentTextArea() const
{
    return textArea1()->hasFocus() ? textArea1() : textArea2();
}

TextArea2Splitter *TextArea2SplitterHandle::splitter() const
{
    return qobject_cast<TextArea2Splitter *>(QSplitterHandle::splitter());
}

void TextArea2SplitterHandle::paintEvent(QPaintEvent *)
{
    QRect rect = contentsRect();
    rect.adjust(0, 0, -2, -4);

    QPainter p(this);
    QStyleOption opt;
    opt.rect = rect;
    opt.palette = palette();
    opt.state = (orientation() == Qt::Horizontal ? QStyle::State_Horizontal : QStyle::State_None)
            | (isEnabled() ? QStyle::State_Enabled : QStyle::State_None)
            | (hasFocus() ? QStyle::State_Sunken : QStyle::State_None);

    parentWidget()->style()->drawControl(QStyle::CE_Splitter, &opt, &p, splitter());
}

void TextArea2SplitterHandle::setupUi()
{
    const auto direction =
            (orientation() == Qt::Horizontal) ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;

    auto layout = new QBoxLayout(direction);
    layout->setContentsMargins(0, 0, 0, 0);

    m_btMoveAllFrom1To2 = ControlUtil::createSplitterButton(":/icons/control_fastforward.png",
            [&] { TextAreaUtil::moveAllLines(textArea1(), textArea2()); });
    m_btMoveSelectedFrom1To2 = ControlUtil::createSplitterButton(":/icons/control_end.png",
            [&] { TextAreaUtil::moveSelectedLines(textArea1(), textArea2()); });
    m_btInterchangeAll = ControlUtil::createSplitterButton(":/icons/control_repeat.png",
            [&] { TextAreaUtil::interchangeAllLines(textArea1(), textArea2()); });
    m_btMoveSelectedFrom2To1 = ControlUtil::createSplitterButton(":/icons/control_start.png",
            [&] { TextAreaUtil::moveSelectedLines(textArea2(), textArea1()); });
    m_btMoveAllFrom2To1 = ControlUtil::createSplitterButton(":/icons/control_rewind.png",
            [&] { TextAreaUtil::moveAllLines(textArea2(), textArea1()); });

    m_buttonsLayout = new QBoxLayout(direction);
    m_buttonsLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonsLayout->setSpacing(10);

    m_buttonsLayout->addWidget(m_btMoveAllFrom1To2, 0, Qt::AlignHCenter);
    m_buttonsLayout->addWidget(m_btMoveSelectedFrom1To2, 0, Qt::AlignHCenter);
    m_buttonsLayout->addWidget(m_btInterchangeAll, 0, Qt::AlignHCenter);
    m_buttonsLayout->addWidget(m_btMoveSelectedFrom2To1, 0, Qt::AlignHCenter);
    m_buttonsLayout->addWidget(m_btMoveAllFrom2To1, 0, Qt::AlignHCenter);

    layout->addStretch();
    layout->addLayout(m_buttonsLayout);
    layout->addStretch();

    this->setLayout(layout);
}
