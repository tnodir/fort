#include "addressesarea.h"

#include <QLabel>

#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/selector/zonesselector.h>

AddressesArea::AddressesArea(
        FirewallConf::AddressList addressList, const QString &iconPath, QWidget *parent) :
    QWidget(parent), m_addressList(addressList)
{
    setupUi(iconPath);
}

void AddressesArea::setZones(quint32 zones)
{
    m_btSelectZones->setZones(zones);
}

void AddressesArea::setText(const QString &text)
{
    const QSignalBlocker blocker(m_editIpText); // textEdited() is by the user only

    m_editIpText->setPlainText(text);
}

void AddressesArea::retranslateUi()
{
    m_labelTitle->setText(FirewallConf::addressListNames().value(m_addressList));

    m_btSelectZones->retranslateUi();
}

void AddressesArea::setupUi(const QString &iconPath)
{
    // Header
    auto headerLayout = setupHeaderLayout(iconPath);

    // Edit IP Text
    setupEditIpText();

    // Layout
    auto layout = ControlUtil::createVLayout();
    layout->addLayout(headerLayout);
    layout->addWidget(m_editIpText, 1);

    this->setLayout(layout);
}

QLayout *AddressesArea::setupHeaderLayout(const QString &iconPath)
{
    // Icon
    const QSize iconSize(16, 16);
    auto iconLabel = ControlUtil::createIconLabel(iconPath, iconSize);

    // Title
    m_labelTitle = ControlUtil::createLabel();

    // Select Zones
    m_btSelectZones = new ZonesSelector();

    connect(m_btSelectZones, &ZonesSelector::zonesChanged, this,
            [&] { emit zonesEdited(m_btSelectZones->zones()); });

    // Layout
    auto layout = ControlUtil::createHLayoutByWidgets(
            { iconLabel, m_labelTitle, /*stretch*/ nullptr, m_btSelectZones });

    return layout;
}

void AddressesArea::setupEditIpText()
{
    m_editIpText = new PlainTextEdit();

    connect(m_editIpText, &QPlainTextEdit::textChanged, this,
            [&] { emit textEdited(m_editIpText->toPlainText()); });
}
