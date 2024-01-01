#include "appscolumn.h"

#include <QLabel>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <util/iconcache.h>

AppsColumn::AppsColumn(const QString &iconPath, QWidget *parent) : QWidget(parent)
{
    setupUi();
    setupIcon(iconPath);
}

void AppsColumn::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Header
    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setSpacing(2);
    layout->addLayout(m_headerLayout);

    m_icon = ControlUtil::createLabel();
    m_icon->setScaledContents(true);
    m_icon->setMaximumSize(16, 16);

    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(ControlUtil::fontDemiBold());
    m_labelTitle->setFixedHeight(24);

    m_headerLayout->addWidget(m_icon);
    m_headerLayout->addWidget(m_labelTitle, 1);

    // Text Area
    setupTextEdit();

    layout->addWidget(m_editText);

    this->setLayout(layout);
}

void AppsColumn::setupTextEdit()
{
    m_editText = new PlainTextEdit();

    connect(m_editText, &QPlainTextEdit::textChanged, this, &AppsColumn::onTextChanged);
}

void AppsColumn::setupIcon(const QString &iconPath)
{
    icon()->setPixmap(IconCache::file(iconPath));
}

void AppsColumn::onTextChanged()
{
    const auto text = m_editText->toPlainText();

    emit textEdited(text);
}
