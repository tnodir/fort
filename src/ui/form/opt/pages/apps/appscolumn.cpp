#include "appscolumn.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "../../../controls/controlutil.h"

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
    m_labelTitle->setFont(ControlUtil::fontDemiBold());

    headerLayout->addWidget(m_labelTitle);

    // Text Area
    m_editText = new QPlainTextEdit();
    layout->addWidget(m_editText);

    this->setLayout(layout);
}
