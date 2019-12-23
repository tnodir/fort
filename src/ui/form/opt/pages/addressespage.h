#ifndef ADDRESSESPAGE_H
#define ADDRESSESPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(AddressGroup)
QT_FORWARD_DECLARE_CLASS(AddressesColumn)
QT_FORWARD_DECLARE_CLASS(TextArea2Splitter)

class AddressesPage : public BasePage
{
    Q_OBJECT
public:
    explicit AddressesPage(OptionsController *ctrl = nullptr,
                           QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupSplitter();
    void setupIncludeAddresses();
    void setupExcludeAddresses();
    void setupAddressesUseAllEnabled();
    void retranslateAddressesPlaceholderText();
    void refreshGroup();
    void setupAddressGroup();

    AddressGroup *addressGroup() const { return m_addressGroup; }
    AddressGroup *addressGroupByIndex(int index) const;

private:
    AddressGroup *m_addressGroup = nullptr;

    QTabBar *m_tabBar = nullptr;
    AddressesColumn *m_includeAddresses = nullptr;
    AddressesColumn *m_excludeAddresses = nullptr;
    TextArea2Splitter *m_splitter = nullptr;
};

#endif // ADDRESSESPAGE_H
