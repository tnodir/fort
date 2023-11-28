#include "appscolumn.h"

#include <QLabel>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>

AppsColumn::AppsColumn(QWidget *parent) : QWidget(parent)
{
    setupUi();
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

    m_headerLayout->addWidget(m_icon);
    m_headerLayout->addWidget(m_labelTitle, 1);

    // Text Area
    m_editText = new PlainTextEdit();
    m_editText->setTabChangesFocus(true);
    layout->addWidget(m_editText);

    this->setLayout(layout);
}
