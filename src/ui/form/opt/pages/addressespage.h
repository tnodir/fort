#ifndef ADDRESSESPAGE_H
#define ADDRESSESPAGE_H

#include <conf/firewallconf.h>

#include "optbasepage.h"

class AddressesArea;

class AddressesPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit AddressesPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    void onResetToDefault() override;

protected slots:
    void onRetranslateUi() override;

private:
    void retranslateEditLanPlaceholderText();

    void setupUi();
    QLayout *setupColumn1();
    QLayout *setupColumn2();

    void setupLanBox();
    void setupEditLanTextActions();

    void setupInetBox();

    AddressesArea *createAddressesArea(
            FirewallConf::AddressList addressList, const QString &iconPath);

    void updateUi();

    QList<AddressesArea *> addressesAreas() const;

private:
    QGroupBox *m_gbLan = nullptr;
    QGroupBox *m_gbInet = nullptr;

    QCheckBox *m_cbFilterLocals = nullptr;
    QCheckBox *m_cbFilterLocalNet = nullptr;
    AddressesArea *m_lanAddresses = nullptr;
    QAction *m_actAddLocalNetworks = nullptr;

    AddressesArea *m_allowInetAddresses = nullptr;
    AddressesArea *m_blockInetAddresses = nullptr;
};

#endif // ADDRESSESPAGE_H
