#include "dnspage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <conf/inioptions.h>
#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>

DnsPage::DnsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void DnsPage::onResetToDefault()
{
    m_cbFilterEnabled->setChecked(false);
    m_editServers->clear();
}

void DnsPage::onRetranslateUi()
{
    m_gbForwarder->setTitle(tr("DNS Forwarder"));

    m_cbFilterEnabled->setText(tr("Filter DNS"));
    m_cbFilterEnabled->setToolTip(
            tr("Resolve only the host names of the enabled Hosts Zones and of the host() filters, "
               "the other names are not found (only with the Service)"));
    m_labelServers->setText(tr("Upstream DNS servers:"));
    m_editServers->setPlaceholderText(tr("Adapters' DNS servers"));
    m_editServers->setToolTip(tr("Comma-separated, e.g. \"1.1.1.1, 9.9.9.9\""));
}

void DnsPage::setupUi()
{
    // DNS Forwarder Group Box
    setupForwarderBox();

    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);
    colLayout->addWidget(m_gbForwarder);
    colLayout->addStretch();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout);
    layout->addStretch();

    this->setLayout(layout);
}

void DnsPage::setupForwarderBox()
{
    m_cbFilterEnabled = ControlUtil::createCheckBox(ini().dnsFilterEnabled(), [&](bool checked) {
        ini().setDnsFilterEnabled(checked);
        ctrl()->setIniEdited();
    });

    // Servers Row
    auto serversLayout = setupServersLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addLayout(serversLayout);

    m_gbForwarder = new QGroupBox();
    m_gbForwarder->setLayout(layout);
}

QLayout *DnsPage::setupServersLayout()
{
    m_labelServers = ControlUtil::createLabel();

    m_editServers = ControlUtil::createLineEdit(ini().dnsServers(), [&](const QString &text) {
        ini().setDnsServers(text);
        ctrl()->setIniEdited();
    });
    m_editServers->setClearButtonEnabled(true);
    m_editServers->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelServers, m_editServers);
}
