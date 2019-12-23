#include "appscolumn.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

AppsColumn::AppsColumn(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void AppsColumn::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setMargin(0);

    // Header
    auto headerLayout = new QHBoxLayout();
    layout->addLayout(headerLayout);

    m_labelTitle = new QLabel();
    {
        QFont font;
        font.setWeight(QFont::DemiBold);
        m_labelTitle->setFont(font);
    }

    headerLayout->addWidget(m_labelTitle);
    headerLayout->addStretch(1);

    // Text Area
    m_editText = new QPlainTextEdit();
    layout->addWidget(m_editText);

    this->setLayout(layout);
}
