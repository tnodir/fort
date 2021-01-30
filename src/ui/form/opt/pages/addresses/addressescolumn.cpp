#include "addressescolumn.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../controls/controlutil.h"
#include "../../../controls/plaintextedit.h"

AddressesColumn::AddressesColumn(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void AddressesColumn::retranslateUi()
{
    m_btSelectZones->setText(tr("Zones"));
    m_btSelectZones->setToolTip(tr("Select Zones"));
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

    m_btSelectZones = ControlUtil::createButton(":/icons/map-map-marker.png");
    layout->addWidget(m_btSelectZones);

    m_labelZones = ControlUtil::createLabel();
    m_labelZones->setWordWrap(true);

    auto font = ControlUtil::fontDemiBold();
    font.setItalic(true);
    m_labelZones->setFont(font);

    layout->addWidget(m_labelZones, 1);

    return layout;
}
