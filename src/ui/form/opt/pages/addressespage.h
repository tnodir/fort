#ifndef ADDRESSESPAGE_H
#define ADDRESSESPAGE_H

#include "optbasepage.h"

class AddressGroup;
class PlainTextEdit;
class ZonesSelector;

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
    QLayout *setupLanHeaderLayout();
    void setupEditLanText();
    void setupEditLanTextActions();

    void setupBlockInetBox();
    QLayout *setupBlockInetHeaderLayout();
    void setupEditBlockInetText();

    void updateUi();

    const QList<AddressGroup *> &addressGroups() const;
    AddressGroup *inetAddressGroup() const;
    AddressGroup *allowAddressGroup() const;

private:
    QGroupBox *m_gbLan = nullptr;
    QGroupBox *m_gbInet = nullptr;

    QCheckBox *m_cbFilterLocals = nullptr;
    QCheckBox *m_cbFilterLocalNet = nullptr;
    QLabel *m_iconLan = nullptr;
    QLabel *m_labelLanText = nullptr;
    ZonesSelector *m_btLanZones = nullptr;
    PlainTextEdit *m_editLanText = nullptr;
    QAction *m_actAddLocalNetworks = nullptr;

    QLabel *m_iconBlockInet = nullptr;
    QLabel *m_labelBlockInetText = nullptr;
    ZonesSelector *m_btBlockInetZones = nullptr;
    PlainTextEdit *m_editBlockInetText = nullptr;
};

#endif // ADDRESSESPAGE_H
