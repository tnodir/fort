#include "appscolumn.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

AppsColumn::AppsColumn(const QString &iconPath, QWidget *parent) : QWidget(parent)
{
    setupUi(iconPath);
}

void AppsColumn::setText(const QString &text)
{
    m_editText->setPlainText(text);

    updateBtClear();
}

void AppsColumn::setupUi(const QString &iconPath)
{
    // Header
    m_headerLayout = setupHeaderLayout(iconPath);

    // Text Area
    setupTextEdit();

    auto layout = ControlUtil::createVLayout();
    layout->addLayout(m_headerLayout);
    layout->addWidget(m_editText);

    this->setLayout(layout);
}

QLayout *AppsColumn::setupHeaderLayout(const QString &iconPath)
{
    // Icon
    const QSize iconSize(16, 16);
    m_icon = ControlUtil::createIconLabel(iconPath, iconSize);

    // Title
    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(GuiUtil::fontBold());
    m_labelTitle->setFixedHeight(24);

    // Clear Button
    m_btClear =
            ControlUtil::createFlatToolButton(":/icons/delete.png", [&] { m_editText->clear(); });

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_icon, m_labelTitle, ControlUtil::createVSeparator(), m_btClear,
                    /*stretch*/ nullptr });
    layout->setSpacing(2);

    return layout;
}

void AppsColumn::setupTextEdit()
{
    m_editText = new PlainTextEdit();

    m_editText->setReadOnly(true); // TODO: COMPAT: Remove AppGroup::*Text() after v4.1.0

    connect(m_editText, &QPlainTextEdit::textChanged, this, &AppsColumn::onTextChanged);
}

void AppsColumn::updateBtClear()
{
    m_btClear->setVisible(!m_editText->isEmpty());
}

void AppsColumn::onTextChanged()
{
    const auto text = m_editText->toPlainText();

    updateBtClear();

    emit textEdited(text);
}
