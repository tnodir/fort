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
    m_btOpenZones->setToolTip(tr("Show Zones"));

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
    auto headerLayout = new QHBoxLayout();
    layout->addLayout(headerLayout);

    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(ControlUtil::fontDemiBold());

    m_cbUseAll = new QCheckBox();

    headerLayout->addWidget(m_labelTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_cbUseAll);

    // Zones Row
    auto zonesLayout = setupZonesRow();
    layout->addLayout(zonesLayout);

    // Text Area
    m_editIpText = new PlainTextEdit();
    m_editIpText->setTabChangesFocus(true);
    layout->addWidget(m_editIpText);

    this->setLayout(layout);
}

QLayout *AddressesColumn::setupZonesRow()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    // Open Zones
    m_btOpenZones = ControlUtil::createLinkButton(":/icons/ip_class.png");

    connect(m_btOpenZones, &QPushButton::clicked, IoC<WindowManager>(),
            &WindowManager::showZonesWindow);

    // Select Zones
    m_btSelectZones = ControlUtil::createButton(QString());

    layout->addWidget(m_btOpenZones);
    layout->addWidget(m_btSelectZones);
    layout->addStretch();

    return layout;
}
