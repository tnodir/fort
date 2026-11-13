#include "dnspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <dns/dnshttpsserver.h>
#include <dns/dnsserverstest.h>
#include <dns/dnsutil.h>

#include <conf/inioptions.h>
#include <dns/dnsmanager.h>
#include <dns/dnsprovider.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/selector/hostzonesselector.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <fortsettings.h>

using namespace Fort;

DnsPage::DnsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void DnsPage::onResetToDefault()
{
    m_cbFilterEnabled->setChecked(false);

    m_btBlockZones->setMask(0);
    onBlockZonesEdited();

    m_cbBlockPorts->setChecked(true);
    m_editServers->clear();
}

void DnsPage::onRetranslateUi()
{
    m_labelNoService->setText(tr("Without the Service, the DNS options don't work"));

    m_gbForwarder->setTitle(tr("DNS Forwarder"));

    m_cbFilterEnabled->setText(tr("Filter DNS"));
    m_cbFilterEnabled->setToolTip(
            tr("Resolve the host names by the upstream DNS servers, adding the addresses of the "
               "Hosts Zones used by the IP Addresses, Programs and Rules and of the host() "
               "filters before answering; the names of the Block Zones are not found (only with "
               "the Service). The programs resolving the names by DNS over HTTPS themselves, "
               "e.g. the browsers with Secure DNS, bypass the DNS Forwarder"));
    updateForwarderErrorWarning();
    m_labelBlockZones->setText(tr("Block Zones:"));
    m_labelBlockZones->setToolTip(tr("The names of the enabled Hosts Zones are not found, even if "
                                     "a host() filter lists them"));
    m_btBlockZones->retranslateUi();
    m_cbBlockPorts->setText(tr("Block ports 53 and 853 for other programs"));
    m_cbBlockPorts->setToolTip(
            tr("Block the connections of the programs other than Fort Firewall's Service to the "
               "DNS servers' port 53 and to the DNS over TLS and over QUIC servers' port 853, "
               "while DNS is filtered: they can't resolve the names past the DNS Forwarder by "
               "these protocols. DNS over HTTPS goes to the port 443 of the web sites, which "
               "isn't blocked"));
    m_labelServers->setText(tr("Upstream DNS servers:"));
    m_comboServers->setItemText(0, adaptersServersText());
    m_comboServers->setPlaceholderText(tr("Custom"));
    m_btTest->setText(tr("Test"));
    m_btTest->setToolTip(tr("Ask each server for \"example.com\" (only with the Service)"));
    m_editServers->setPlaceholderText(adaptersServersText());
    m_editServers->setToolTip(
            tr("A server per line: addresses, e.g. \"1.1.1.1\", or DNS over HTTPS URLs with the "
               "server's address, e.g. \"https://dns.example/dns-query?ip=192.0.2.1\""));
    m_labelMixedServers->setText(mixedServersText());
    updateTestResults();
}

void DnsPage::setupUi()
{
    // No Service's Warning
    m_labelNoService = ControlUtil::createLabel();

    auto noServiceWarning = createNoServiceWarning(m_labelNoService);

    // DNS Forwarder Group Box
    setupForwarderBox();

    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);
    colLayout->addWidget(noServiceWarning);
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

    setupForwarderErrorWarning();

    // Block Zones Row
    auto blockZonesLayout = setupBlockZonesLayout();

    m_cbBlockPorts = ControlUtil::createCheckBox(ini().dnsBlockPorts(), [&](bool checked) {
        ini().setDnsBlockPorts(checked);
        ctrl()->setIniEdited();
    });

    // Servers Row
    auto serversLayout = setupServersLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addWidget(m_forwarderErrorWarning);
    layout->addLayout(blockZonesLayout);
    layout->addWidget(m_cbBlockPorts);
    layout->addLayout(serversLayout);

    m_gbForwarder = new QGroupBox();
    m_gbForwarder->setLayout(layout);
}

void DnsPage::setupForwarderErrorWarning()
{
    m_labelForwarderError = ControlUtil::createLabel();

    m_forwarderErrorWarning = createWarning(m_labelForwarderError);

    // Only with the Service
    if (auto dnsManager = Fort::dnsManager()) {
        connect(dnsManager, &DnsManager::statusChanged, this,
                &DnsPage::updateForwarderErrorWarning);
    }

    updateForwarderErrorWarning();
}

QLayout *DnsPage::setupBlockZonesLayout()
{
    m_labelBlockZones = ControlUtil::createLabel();

    m_btBlockZones = new HostZonesSelector();
    m_btBlockZones->setMask(ini().dnsBlockZones());

    connect(m_btBlockZones, &HostZonesSelector::maskChanged, this, &DnsPage::onBlockZonesEdited);

    return ControlUtil::createHLayoutByWidgets({ m_labelBlockZones, m_btBlockZones }, /*margin=*/0);
}

QLayout *DnsPage::setupServersLayout()
{
    m_labelServers = ControlUtil::createLabel();

    setupComboServers();
    setupTestButton();
    setupEditServers();
    setupMixedServersWarning();
    setupTestResults();

    auto layout = new QVBoxLayout();
    layout->addLayout(ControlUtil::createHLayoutByWidgets(
            { m_labelServers, /*stretch*/ nullptr, m_comboServers, m_btTest }, /*margin=*/0));
    layout->addWidget(m_editServers);
    layout->addWidget(m_mixedServersWarning);
    layout->addWidget(m_editTestResults);

    return layout;
}

void DnsPage::setupComboServers()
{
    m_comboServers = ControlUtil::createComboBox({}, [&](int index) {
        m_editServers->setPlainText(m_comboServers->itemData(index).toStringList().join('\n'));
    });

    m_comboServers->addItem(QString(), QStringList());

    for (const DnsProvider &provider : DnsProvider::readList()) {
        m_comboServers->addItem(provider.title, provider.servers);
    }
}

void DnsPage::setupTestButton()
{
    m_btTest = ControlUtil::createButton(":/icons/play.png", [&] { testServers(); });

    m_btTest->setEnabled(settings()->hasService());
}

void DnsPage::setupEditServers()
{
    const QStringList iniServers = ini().dnsServers();

    m_editServers = new PlainTextEdit();
    m_editServers->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editServers->setMinimumWidth(400);
    m_editServers->setFixedHeight(80); // 4 lines
    m_editServers->setText(iniServers.join('\n'));

    m_comboServers->setCurrentIndex(m_comboServers->findData(iniServers));

    connect(m_editServers, &QPlainTextEdit::textChanged, this, [&] {
        const QStringList servers = DnsUtil::splitServers(m_editServers->toPlainText());

        ini().setDnsServers(servers);
        ctrl()->setIniEdited();

        m_comboServers->setCurrentIndex(m_comboServers->findData(servers));

        updateMixedServersWarning(servers);
    });
}

void DnsPage::setupMixedServersWarning()
{
    m_labelMixedServers = ControlUtil::createLabel();

    m_mixedServersWarning = createWarning(m_labelMixedServers);

    updateMixedServersWarning(ini().dnsServers());
}

void DnsPage::setupTestResults()
{
    m_editTestResults = new PlainTextEdit();
    m_editTestResults->setReadOnly(true);
    m_editTestResults->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editTestResults->setFixedHeight(80); // 4 lines
    m_editTestResults->setVisible(false);
}

void DnsPage::onBlockZonesEdited()
{
    ini().setDnsBlockZones(m_btBlockZones->mask());
    ctrl()->setIniEdited();
}

// Of the saved options
void DnsPage::updateForwarderErrorWarning()
{
    auto dnsManager = Fort::dnsManager();
    const DnsForwarderStatus status = dnsManager ? dnsManager->status() : DnsForwarderStatus();

    m_labelForwarderError->setText(forwarderErrorText(status));
    m_forwarderErrorWarning->setVisible(status.isError());
}

void DnsPage::updateMixedServersWarning(const QStringList &servers)
{
    m_mixedServersWarning->setVisible(DnsHttpsServer::isMixedServers(servers));
}

void DnsPage::testServers()
{
    delete m_serversTest;

    m_serversTest = dnsManager()->createServersTest(ini().dnsServers(), this);

    connect(m_serversTest, &DnsServersTest::resultReady, this, &DnsPage::updateTestResults);

    m_serversTest->start();

    updateTestResults();

    m_editTestResults->setVisible(true);
}

void DnsPage::updateTestResults()
{
    if (!m_serversTest)
        return;

    const QStringList lines = m_serversTest->resultLines(&serverResultText, adaptersServersText());

    m_editTestResults->setPlainText(lines.join('\n'));
}

QString DnsPage::forwarderErrorText(const DnsForwarderStatus &status)
{
    switch (status.status) {
    case DnsForwarderStatus::StatusNrptError:
        return tr("The system's DNS queries aren't sent to the DNS Forwarder: the rule of the Name "
                  "Resolution Policy Table can't be added");
    case DnsForwarderStatus::StatusMixedServers:
        return tr("The DNS Forwarder isn't started: the saved upstream DNS servers mix plain and "
                  "DNS over HTTPS ones");
    case DnsForwarderStatus::StatusInvalidServers:
        return tr("The DNS Forwarder isn't started: the saved upstream DNS servers are invalid");
    case DnsForwarderStatus::StatusListenError:
        return tr("The DNS Forwarder isn't started: can't listen on the port 53: %1")
                .arg(status.errorMessage);
    default:
        return {};
    }
}

QString DnsPage::serverResultText(const DnsServerResult &result)
{
    switch (result.status) {
    case DnsServerResult::StatusAnswered:
        return serverAnsweredText(result);
    case DnsServerResult::StatusFailed:
        return tr("Error: %1").arg(result.error);
    default:
        return serverStatusText(result);
    }
}

QString DnsPage::serverStatusText(const DnsServerResult &result)
{
    switch (result.status) {
    case DnsServerResult::StatusNone:
        return tr("Testing...");
    case DnsServerResult::StatusTimeout:
        return tr("Timeout");
    case DnsServerResult::StatusLoop:
        return tr("Loop: the server is the DNS Forwarder itself");
    case DnsServerResult::StatusInvalid:
        return tr("Invalid server");
    case DnsServerResult::StatusMixed:
        return mixedServersText();
    default:
        return {};
    }
}

QString DnsPage::serverAnsweredText(const DnsServerResult &result)
{
    const QString timeText = tr("%1 ms").arg(result.msecs);

    return result.isNoError() ? timeText : DnsUtil::rcodeText(result.rcode) + ", " + timeText;
}

QString DnsPage::adaptersServersText()
{
    return tr("Adapters' DNS servers");
}

QString DnsPage::mixedServersText()
{
    return tr("Plain and DNS over HTTPS servers can't be mixed");
}
