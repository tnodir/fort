#include "dnspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <dns/dnsserverstest.h>
#include <dns/dnsutil.h>

#include <conf/inioptions.h>
#include <dns/dnsmanager.h>
#include <dns/dnsprovider.h>
#include <form/controls/controlutil.h>
#include <form/controls/plaintextedit.h>
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
    m_comboServers->setItemText(0, adaptersServersText());
    m_comboServers->setPlaceholderText(tr("Custom"));
    m_btTest->setText(tr("Test"));
    m_btTest->setToolTip(tr("Ask each server for \"example.com\" (only with the Service)"));
    m_editServers->setPlaceholderText(adaptersServersText());
    m_editServers->setToolTip(
            tr("A server per line: addresses, e.g. \"1.1.1.1\", or DNS over HTTPS URLs with the "
               "server's address, e.g. \"https://dns.example/dns-query?ip=192.0.2.1\""));
    updateTestResults();
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

    setupComboServers();
    setupTestButton();
    setupEditServers();
    setupTestResults();

    auto layout = new QVBoxLayout();
    layout->addLayout(ControlUtil::createHLayoutByWidgets(
            { m_labelServers, /*stretch*/ nullptr, m_comboServers, m_btTest }, /*margin=*/0));
    layout->addWidget(m_editServers);
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
    });
}

void DnsPage::setupTestResults()
{
    m_editTestResults = new PlainTextEdit();
    m_editTestResults->setReadOnly(true);
    m_editTestResults->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editTestResults->setFixedHeight(80); // 4 lines
    m_editTestResults->setVisible(false);
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

QString DnsPage::serverResultText(const DnsServerResult &result)
{
    const QString timeText = tr("%1 ms").arg(result.msecs);

    switch (result.status) {
    case DnsServerResult::StatusNone:
        return tr("Testing...");
    case DnsServerResult::StatusAnswered:
        return result.isNoError() ? timeText : DnsUtil::rcodeText(result.rcode) + ", " + timeText;
    case DnsServerResult::StatusFailed:
        return tr("Error: %1").arg(result.error);
    case DnsServerResult::StatusTimeout:
        return tr("Timeout");
    case DnsServerResult::StatusInvalid:
        return tr("Invalid server");
    default:
        return {};
    }
}

QString DnsPage::adaptersServersText()
{
    return tr("Adapters' DNS servers");
}
