#include "addressescolumn.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

AddressesColumn::AddressesColumn(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void AddressesColumn::setZonesCount(qint8 v)
{
    if (m_zonesCount == v)
        return;

    m_zonesCount = v;

    retranslateZonesText();
}

void AddressesColumn::retranslateUi()
{
    retranslateZonesText();
    m_btSelectZones->setToolTip(tr("Select Zones"));
}

void AddressesColumn::retranslateZonesText()
{
    m_btSelectZones->setText(tr("Zones") + QString(" (%1)").arg(zonesCount()));
}

void AddressesColumn::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Header
    auto headerLayout = setupHeaderLayout();
    layout->addLayout(headerLayout);

    // Text Area
    m_editIpText = new PlainTextEdit();
    m_editIpText->setTabChangesFocus(true);
    layout->addWidget(m_editIpText);

    this->setLayout(layout);
}

QLayout *AddressesColumn::setupHeaderLayout()
{
    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(ControlUtil::fontDemiBold());

    // Use All
    m_cbUseAll = new QCheckBox();

    // Select Zones
    m_btSelectZones = ControlUtil::createButton(":/icons/ip_class.png");

    auto layout = new QHBoxLayout();
    layout->addWidget(m_labelTitle);
    layout->addStretch();
    layout->addWidget(m_cbUseAll);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_btSelectZones);

    return layout;
}
