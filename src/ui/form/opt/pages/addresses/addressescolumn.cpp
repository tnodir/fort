#include "addressescolumn.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "../../../controls/controlutil.h"
#include "../../../controls/plaintextedit.h"

AddressesColumn::AddressesColumn(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void AddressesColumn::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setMargin(0);

    // Header
    auto headerLayout = new QHBoxLayout();
    layout->addLayout(headerLayout);

    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(ControlUtil::fontDemiBold());

    m_cbUseAll = new QCheckBox();

    headerLayout->addWidget(m_labelTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_cbUseAll);

    // Text Area
    m_editIpText = new PlainTextEdit();
    m_editIpText->setTabChangesFocus(true);
    layout->addWidget(m_editIpText);

    this->setLayout(layout);
}
