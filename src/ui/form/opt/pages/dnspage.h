#ifndef DNSPAGE_H
#define DNSPAGE_H

#include "optbasepage.h"

class DnsServersTest;
class HostZonesSelector;
class PlainTextEdit;

struct DnsForwarderStatus;
struct DnsServerResult;

class DnsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit DnsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    QString helpSection() const override { return "dns-tab"; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupForwarderBox();
    void setupForwarderErrorWarning();
    QLayout *setupBlockZonesLayout();
    QLayout *setupServersLayout();
    void setupComboServers();
    void setupTestButton();
    void setupEditServers();
    void setupMixedServersWarning();
    void setupTestResults();

    void onBlockZonesEdited();

    void updateForwarderErrorWarning();
    void updateMixedServersWarning(const QStringList &servers);

    void testServers();
    void updateTestResults();

    static QString forwarderErrorText(const DnsForwarderStatus &status);
    static QString serverResultText(const DnsServerResult &result);
    static QString serverStatusText(const DnsServerResult &result);
    static QString serverAnsweredText(const DnsServerResult &result);
    static QString adaptersServersText();
    static QString mixedServersText();

private:
    QLabel *m_labelNoService = nullptr;

    QGroupBox *m_gbForwarder = nullptr;

    QCheckBox *m_cbFilterEnabled = nullptr;
    QLabel *m_labelForwarderError = nullptr;
    QWidget *m_forwarderErrorWarning = nullptr;
    QLabel *m_labelBlockZones = nullptr;
    HostZonesSelector *m_btBlockZones = nullptr;
    QCheckBox *m_cbBlockPorts = nullptr;
    QLabel *m_labelServers = nullptr;
    QComboBox *m_comboServers = nullptr;
    QPushButton *m_btTest = nullptr;
    PlainTextEdit *m_editServers = nullptr;
    QLabel *m_labelMixedServers = nullptr;
    QWidget *m_mixedServersWarning = nullptr;
    PlainTextEdit *m_editTestResults = nullptr;

    DnsServersTest *m_serversTest = nullptr;
};

#endif // DNSPAGE_H
