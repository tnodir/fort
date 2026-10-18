#include "addressespage.h"

#include <QAction>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/addressgroup.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/net/netutil.h>
#include <util/textareautil.h>

#include "addresses/addressesarea.h"

using namespace Fort;

AddressesPage::AddressesPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();

    updateUi();
}

void AddressesPage::onResetToDefault()
{
    m_cbFilterLocals->setChecked(false);
    m_cbFilterLocalNet->setChecked(false);

    for (auto addressGroup : conf().addressGroups()) {
        addressGroup->resetToDefault();
    }

    conf().setupDefaultAddressGroups();
    ctrl()->setOptEdited();

    updateUi();
}

void AddressesPage::onRetranslateUi()
{
    m_gbLan->setTitle(tr("Local Area Network"));
    m_gbInet->setTitle(tr("Internet"));

    m_cbFilterLocals->setText(tr("Filter Local Addresses") + " (127.0.0.0/8, 255.255.255.255)");
    m_cbFilterLocals->setToolTip(
            tr("Filter Local Loopback (127.0.0.0/8) and Broadcast (255.255.255.255) Addresses"));
    m_cbFilterLocalNet->setText(tr("Filter Local Network"));

    for (AddressesArea *addressesArea : addressesAreas()) {
        addressesArea->retranslateUi();
    }

    m_actAddLocalNetworks->setText(tr("Add Local Networks"));
    retranslateEditLanPlaceholderText();
}

void AddressesPage::retranslateEditLanPlaceholderText()
{
    const auto placeholderText = tr("# Examples:") + '\n' + NetUtil::localIpNetworksText(9);

    m_lanAddresses->editIpText()->setPlaceholderText(placeholderText);
}

void AddressesPage::setupUi()
{
    // Column #1
    auto colLayout1 = setupColumn1();

    // Column #2
    auto colLayout2 = setupColumn2();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();

    this->setLayout(layout);
}

QLayout *AddressesPage::setupColumn1()
{
    // LAN Group Box
    setupLanBox();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_gbLan, 1);

    return layout;
}

QLayout *AddressesPage::setupColumn2()
{
    // Inet Box
    setupInetBox();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_gbInet, 1);

    return layout;
}

void AddressesPage::setupLanBox()
{
    m_cbFilterLocals = ControlUtil::createCheckBox(conf().filterLocals(), [&](bool checked) {
        conf().setFilterLocals(checked);
        ctrl()->setFlagsEdited();
    });

    m_cbFilterLocalNet = ControlUtil::createCheckBox(conf().filterLocalNet(), [&](bool checked) {
        conf().setFilterLocalNet(checked);
        ctrl()->setFlagsEdited();
    });

    // LAN Addresses
    m_lanAddresses = createAddressesArea(FirewallConf::AddressListLan, ":/icons/hostname.png");
    setupEditLanTextActions();

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbFilterLocals, m_cbFilterLocalNet, ControlUtil::createHSeparator() });
    layout->addWidget(m_lanAddresses, 1);

    m_gbLan = new QGroupBox();
    m_gbLan->setLayout(layout);
}

void AddressesPage::setupEditLanTextActions()
{
    m_actAddLocalNetworks = new QAction(IconCache::icon(":/icons/hostname.png"), QString(), this);

    connect(m_actAddLocalNetworks, &QAction::triggered, this, [&] {
        TextAreaUtil::appendText(m_lanAddresses->editIpText(), NetUtil::localIpNetworksText());
    });

    m_lanAddresses->editIpText()->addContextAction(m_actAddLocalNetworks);
}

void AddressesPage::setupInetBox()
{
    // Allow Inet Addresses
    m_allowInetAddresses =
            createAddressesArea(FirewallConf::AddressListAllowInet, ":/icons/global_telecom.png");

    // Block Inet Addresses
    m_blockInetAddresses =
            createAddressesArea(FirewallConf::AddressListBlockInet, ":/icons/ip_block.png");

    // Layout
    auto layout = ControlUtil::createVLayout(/*margin=*/-1);
    layout->addWidget(m_allowInetAddresses, 1);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_blockInetAddresses, 1);

    m_gbInet = new QGroupBox();
    m_gbInet->setLayout(layout);
}

AddressesArea *AddressesPage::createAddressesArea(
        FirewallConf::AddressList addressList, const QString &iconPath)
{
    auto addressesArea = new AddressesArea(addressList, iconPath);

    connect(addressesArea, &AddressesArea::zonesEdited, this, [=, this](quint32 zones) {
        conf().setAddressListZones(addressList, zones);
        ctrl()->setOptEdited();
    });

    connect(addressesArea, &AddressesArea::textEdited, this, [=, this](const QString &text) {
        conf().setAddressListText(addressList, text);
        ctrl()->setOptEdited();
    });

    return addressesArea;
}

void AddressesPage::updateUi()
{
    for (AddressesArea *addressesArea : addressesAreas()) {
        const auto addressList = addressesArea->addressList();

        addressesArea->setZones(conf().addressListZones(addressList));
        addressesArea->setText(conf().addressListText(addressList));
    }
}

QList<AddressesArea *> AddressesPage::addressesAreas() const
{
    return { m_lanAddresses, m_allowInetAddresses, m_blockInetAddresses };
}
