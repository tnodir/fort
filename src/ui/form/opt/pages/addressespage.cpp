#include "addressespage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/addressgroup.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/textarea2splitter.h>
#include <form/controls/textarea2splitterhandle.h>
#include <form/controls/zonesselector.h>
#include <form/opt/optionscontroller.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/net/netutil.h>
#include <util/textareautil.h>

#include "addresses/addressescolumn.h"

AddressesPage::AddressesPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();

    setupZones();
    setupAddressGroup();
}

AddressGroup *AddressesPage::addressGroup() const
{
    return addressGroupByIndex(addressGroupIndex());
}

void AddressesPage::setAddressGroupIndex(int v)
{
    if (m_addressGroupIndex != v) {
        m_addressGroupIndex = v;
        emit addressGroupChanged();
    }
}

void AddressesPage::onSaveWindowState(IniUser *ini)
{
    ini->setOptWindowAddrSplit(m_splitter->saveState());
}

void AddressesPage::onRestoreWindowState(IniUser *ini)
{
    m_splitter->restoreState(ini->optWindowAddrSplit());
}

void AddressesPage::onRetranslateUi()
{
    m_tabBar->setTabText(0, tr("Internet Addresses"));
    m_tabBar->setTabText(1, tr("Allowed Internet Addresses"));

    m_includeAddresses->labelTitle()->setText(tr("Include"));
    m_includeAddresses->cbUseAll()->setText(tr("Include All"));
    m_includeAddresses->retranslateUi();

    m_excludeAddresses->labelTitle()->setText(tr("Exclude"));
    m_excludeAddresses->cbUseAll()->setText(tr("Exclude All"));
    m_excludeAddresses->retranslateUi();

    auto splitterHandle = m_splitter->handle();
    splitterHandle->btMoveAllFrom1To2()->setToolTip(tr("Move All Lines to 'Exclude'"));
    splitterHandle->btMoveAllFrom2To1()->setToolTip(tr("Move All Lines to 'Include'"));
    splitterHandle->btInterchangeAll()->setToolTip(tr("Interchange All Lines"));
    splitterHandle->btMoveSelectedFrom1To2()->setToolTip(tr("Move Selected Lines to 'Exclude'"));
    splitterHandle->btMoveSelectedFrom2To1()->setToolTip(tr("Move Selected Lines to 'Include'"));
    m_btAddLocals->setToolTip(tr("Add Local Networks"));

    retranslateAddressesPlaceholderText();
}

void AddressesPage::retranslateAddressesPlaceholderText()
{
    const auto placeholderText = tr("# Examples:") + '\n' + NetUtil::localIpNetworksText();

    m_excludeAddresses->editIpText()->setPlaceholderText(placeholderText);
}

void AddressesPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Tab Bar
    m_tabBar = new QTabBar();
    m_tabBar->setShape(QTabBar::RoundedNorth);
    layout->addWidget(m_tabBar);

    m_tabBar->addTab(IconCache::icon(":/icons/global_telecom.png"), QString());
    m_tabBar->addTab(IconCache::icon(":/icons/ip_block.png"), QString());

    // Address Columns
    setupAddressColumns();

    setupAddressesUseAllEnabled();

    // Splitter
    setupSplitter();
    layout->addWidget(m_splitter, 1);

    // Splitter Buttons
    setupSplitterButtons();

    this->setLayout(layout);
}

void AddressesPage::setupAddressColumns()
{
    m_includeAddresses = setupAddressColumn(/*include=*/true);
    m_excludeAddresses = setupAddressColumn(/*include=*/false);
}

AddressesColumn *AddressesPage::setupAddressColumn(bool include)
{
    auto addressesColumn = new AddressesColumn();

    connect(addressesColumn->cbUseAll(), &QCheckBox::toggled, this, [=, this](bool checked) {
        if (include) {
            addressGroup()->setIncludeAll(checked);
        } else {
            addressGroup()->setExcludeAll(checked);
        }

        checkAddressGroupEdited();
    });
    connect(addressesColumn->editIpText(), &QPlainTextEdit::textChanged, this, [=, this] {
        const auto ipText = addressesColumn->editIpText()->toPlainText();

        if (include) {
            addressGroup()->setIncludeText(ipText);
        } else {
            addressGroup()->setExcludeText(ipText);
        }

        checkAddressGroupEdited();
    });

    return addressesColumn;
}

void AddressesPage::setupAddressesUseAllEnabled()
{
    const auto refreshUseAllEnabled = [&] {
        auto cbIncludeAll = m_includeAddresses->cbUseAll();
        auto cbExcludeAll = m_excludeAddresses->cbUseAll();

        const bool includeAll = cbIncludeAll->isChecked();
        const bool excludeAll = cbExcludeAll->isChecked();

        cbIncludeAll->setEnabled(includeAll || !excludeAll);
        cbExcludeAll->setEnabled(!includeAll || excludeAll);
    };

    refreshUseAllEnabled();

    connect(m_includeAddresses->cbUseAll(), &QCheckBox::toggled, this, refreshUseAllEnabled);
    connect(m_excludeAddresses->cbUseAll(), &QCheckBox::toggled, this, refreshUseAllEnabled);
}

void AddressesPage::setupSplitter()
{
    m_splitter = new TextArea2Splitter();

    Q_ASSERT(!m_splitter->handle());

    m_splitter->addWidget(m_includeAddresses);
    m_splitter->addWidget(m_excludeAddresses);

    auto splitterHandle = m_splitter->handle();
    Q_ASSERT(splitterHandle);

    splitterHandle->setTextArea1(m_includeAddresses->editIpText());
    splitterHandle->setTextArea2(m_excludeAddresses->editIpText());
}

void AddressesPage::setupSplitterButtons()
{
    m_btAddLocals = ControlUtil::createSplitterButton(":/icons/hostname.png", [&] {
        auto area = m_splitter->handle()->currentTextArea();
        TextAreaUtil::appendText(area, NetUtil::localIpNetworksText());
    });

    const auto layout = m_splitter->handle()->buttonsLayout();
    layout->addWidget(m_btAddLocals, 0, Qt::AlignHCenter);
}

void AddressesPage::setupZones()
{
    connect(m_includeAddresses->btSelectZones(), &ZonesSelector::zonesChanged, this, [&] {
        const quint32 zones = m_includeAddresses->btSelectZones()->zones();

        addressGroup()->setIncludeZones(zones);

        checkAddressGroupEdited();
    });

    connect(m_excludeAddresses->btSelectZones(), &ZonesSelector::zonesChanged, this, [&] {
        const quint32 zones = m_excludeAddresses->btSelectZones()->zones();

        addressGroup()->setExcludeZones(zones);

        checkAddressGroupEdited();
    });
}

void AddressesPage::setupAddressGroup()
{
    connect(this, &AddressesPage::addressGroupChanged, this, &AddressesPage::updateGroup);

    const auto refreshAddressGroup = [&] {
        const int tabIndex = m_tabBar->currentIndex();
        setAddressGroupIndex(tabIndex);
    };

    refreshAddressGroup();

    connect(m_tabBar, &QTabBar::currentChanged, this, refreshAddressGroup);
}

void AddressesPage::updateGroup()
{
    m_includeAddresses->cbUseAll()->setChecked(addressGroup()->includeAll());
    m_includeAddresses->editIpText()->setText(addressGroup()->includeText());

    m_excludeAddresses->cbUseAll()->setChecked(addressGroup()->excludeAll());
    m_excludeAddresses->editIpText()->setText(addressGroup()->excludeText());

    m_includeAddresses->btSelectZones()->setZones(addressGroup()->includeZones());
    m_excludeAddresses->btSelectZones()->setZones(addressGroup()->excludeZones());
}

void AddressesPage::checkAddressGroupEdited()
{
    if (addressGroup()->edited()) {
        ctrl()->setOptEdited();
    }
}

const QList<AddressGroup *> &AddressesPage::addressGroups() const
{
    return conf()->addressGroups();
}

AddressGroup *AddressesPage::addressGroupByIndex(int index) const
{
    return addressGroups().at(index);
}
