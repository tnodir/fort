#include "fontselector.h"

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/toolbutton.h>
#include <form/dialog/dialogutil.h>
#include <util/guiutil.h>

namespace {

QString fontText(const QFont &font)
{
    return QString("%1, %2 pt").arg(font.family()).arg(font.pointSize());
}

}

FontSelector::FontSelector(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void FontSelector::setDefaultFont(const QFont &font)
{
    m_defaultFont = font;

    m_editFont->setPlaceholderText(fontText(font));

    updateFontText();
}

void FontSelector::setSelectedFont(const QFont &font)
{
    m_selectedFont = font;

    updateFontText();
}

void FontSelector::retranslateUi()
{
    m_btSelectFont->setToolTip(tr("Select Font"));
}

void FontSelector::updateFontText()
{
    // The default font is shown by the placeholder
    const bool isDefault = GuiUtil::isSameFont(m_selectedFont, m_defaultFont);

    m_editFont->setStartText(isDefault ? QString() : fontText(m_selectedFont));
}

void FontSelector::setupUi()
{
    m_editFont = new LineEdit();
    m_editFont->setFocusPolicy(Qt::NoFocus);
    m_editFont->setContextMenuPolicy(Qt::PreventContextMenu);
    m_editFont->setClearButtonEnabled(true);

    connect(m_editFont, &QLineEdit::textEdited, this, [&](const QString &text) {
        if (text.isEmpty()) {
            m_selectedFont = m_defaultFont;
            emit fontSelected(m_selectedFont);
        }
    });

    m_btSelectFont =
            ControlUtil::createIconToolButton(":/icons/font.png", [&] { selectFontDialog(); });

    auto layout = ControlUtil::createRowLayout(m_editFont, m_btSelectFont);
    layout->setSpacing(0);

    this->setLayout(layout);
}

void FontSelector::selectFontDialog()
{
    QFont font = m_selectedFont;

    if (!DialogUtil::getFont(font, m_btSelectFont->toolTip()))
        return;

    setSelectedFont(font);

    emit fontSelected(font);
}
