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
#include <form/controls/zonesselector.h>
#include <form/opt/optionscontroller.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/net/netutil.h>
#include <util/textareautil.h>

AddressesPage::AddressesPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();

    updateUi();
}

void AddressesPage::onResetToDefault()
{
    m_cbFilterLocals->setChecked(false);
    m_cbFilterLocalNet->setChecked(false);

    for (auto addressGroup : addressGroups()) {
        addressGroup->resetToDefault();
    }

    conf()->setupDefaultAddressGroups();

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

    m_labelLanText->setText(tr("Local Network Addresses:"));
    m_btLanZones->retranslateUi();
    m_actAddLocalNetworks->setText(tr("Add Local Networks"));
    retranslateEditLanPlaceholderText();

    m_labelBlockInetText->setText(tr("Block Addresses:"));
    m_btBlockInetZones->retranslateUi();
}

void AddressesPage::retranslateEditLanPlaceholderText()
{
    const auto placeholderText = tr("# Examples:") + '\n' + NetUtil::localIpNetworksText(9);

    m_editLanText->setPlaceholderText(placeholderText);
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
    // Block Inet Box
    setupBlockInetBox();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_gbInet, 1);

    return layout;
}

void AddressesPage::setupLanBox()
{
    m_cbFilterLocals = ControlUtil::createCheckBox(conf()->filterLocals(), [&](bool checked) {
        conf()->setFilterLocals(checked);
        ctrl()->setFlagsEdited();
    });

    m_cbFilterLocalNet = ControlUtil::createCheckBox(conf()->filterLocalNet(), [&](bool checked) {
        conf()->setFilterLocalNet(checked);
        ctrl()->setFlagsEdited();
    });

    // LAN Text Header
    auto lanHeaderLayout = setupLanHeaderLayout();

    // Edit LAN Text
    setupEditLanText();
    setupEditLanTextActions();

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbFilterLocals, m_cbFilterLocalNet, ControlUtil::createHSeparator() });
    layout->addLayout(lanHeaderLayout);
    layout->addWidget(m_editLanText, 1);

    m_gbLan = new QGroupBox();
    m_gbLan->setLayout(layout);
}

QLayout *AddressesPage::setupLanHeaderLayout()
{
    // LAN Icon
    const QSize iconSize(16, 16);
    m_iconLan = ControlUtil::createIconLabel(":/icons/hostname.png", iconSize);

    // LAN Label
    m_labelLanText = ControlUtil::createLabel();

    // Select Zones
    m_btLanZones = new ZonesSelector();

    connect(m_btLanZones, &ZonesSelector::zonesChanged, this, [&] {
        inetAddressGroup()->setExcludeZones(m_btLanZones->zones());
        ctrl()->setOptEdited();
    });

    // Layout
    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_iconLan, m_labelLanText, /*stretch*/ nullptr, m_btLanZones });

    return layout;
}

void AddressesPage::setupEditLanText()
{
    m_editLanText = new PlainTextEdit();

    connect(m_editLanText, &QPlainTextEdit::textChanged, this, [&] {
        const auto text = m_editLanText->toPlainText();

        AddressGroup *inetGroup = inetAddressGroup();
        if (inetGroup->excludeText() != text) {
            inetGroup->setExcludeText(text);
            ctrl()->setOptEdited();
        }
    });
}

void AddressesPage::setupEditLanTextActions()
{
    m_actAddLocalNetworks = new QAction(IconCache::icon(":/icons/hostname.png"), QString(), this);

    connect(m_actAddLocalNetworks, &QAction::triggered, this,
            [&] { TextAreaUtil::appendText(m_editLanText, NetUtil::localIpNetworksText()); });

    m_editLanText->addContextAction(m_actAddLocalNetworks);
}

void AddressesPage::setupBlockInetBox()
{
    // LAN Text Header
    auto blockInetHeaderLayout = setupBlockInetHeaderLayout();

    // Edit LAN Text
    setupEditBlockInetText();

    // Layout
    auto layout = ControlUtil::createVLayout(/*margin=*/-1);
    layout->addLayout(blockInetHeaderLayout);
    layout->addWidget(m_editBlockInetText, 1);

    m_gbInet = new QGroupBox();
    m_gbInet->setLayout(layout);
}

QLayout *AddressesPage::setupBlockInetHeaderLayout()
{
    // Block Inet Icon
    const QSize iconSize(16, 16);
    m_iconBlockInet = ControlUtil::createIconLabel(":/icons/ip_block.png", iconSize);

    // Block Inet Label
    m_labelBlockInetText = ControlUtil::createLabel();

    // Select Zones
    m_btBlockInetZones = new ZonesSelector();

    connect(m_btBlockInetZones, &ZonesSelector::zonesChanged, this, [&] {
        allowAddressGroup()->setExcludeZones(m_btBlockInetZones->zones());
        ctrl()->setOptEdited();
    });

    // Layout
    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_iconBlockInet, m_labelBlockInetText, /*stretch*/ nullptr, m_btBlockInetZones });

    return layout;
}

void AddressesPage::setupEditBlockInetText()
{
    m_editBlockInetText = new PlainTextEdit();

    connect(m_editBlockInetText, &QPlainTextEdit::textChanged, this, [&] {
        const auto text = m_editBlockInetText->toPlainText();

        AddressGroup *allowGroup = allowAddressGroup();
        if (allowGroup->excludeText() != text) {
            allowGroup->setExcludeText(text);
            ctrl()->setOptEdited();
        }
    });
}

void AddressesPage::updateUi()
{
    m_btLanZones->setZones(inetAddressGroup()->excludeZones());
    m_editLanText->setPlainText(inetAddressGroup()->excludeText());

    m_btBlockInetZones->setZones(allowAddressGroup()->excludeZones());
    m_editBlockInetText->setPlainText(allowAddressGroup()->excludeText());
}

const QList<AddressGroup *> &AddressesPage::addressGroups() const
{
    return conf()->addressGroups();
}

AddressGroup *AddressesPage::inetAddressGroup() const
{
    return addressGroups().first();
}

AddressGroup *AddressesPage::allowAddressGroup() const
{
    return addressGroups().at(1);
}
