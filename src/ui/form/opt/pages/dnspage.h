#ifndef DNSPAGE_H
#define DNSPAGE_H

#include "optbasepage.h"

class PlainTextEdit;

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
    QLayout *setupServersLayout();
    void setupComboServers();
    void setupEditServers();

    static QString adaptersServersText();

private:
    QGroupBox *m_gbForwarder = nullptr;

    QCheckBox *m_cbFilterEnabled = nullptr;
    QLabel *m_labelServers = nullptr;
    QComboBox *m_comboServers = nullptr;
    PlainTextEdit *m_editServers = nullptr;
};

#endif // DNSPAGE_H
