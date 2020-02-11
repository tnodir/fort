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

    AddressGroup *addressGroup() const;

    int addressGroupIndex() const { return m_addressGroupIndex; }
    void setAddressGroupIndex(int v);

signals:
    void addressGroupChanged();

protected slots:
    void onSaveWindowState() override;
    void onRestoreWindowState() override;

    void onRetranslateUi() override;

private:
    void retranslateAddressesPlaceholderText();

    void setupUi();
    void setupIncludeAddresses();
    void setupExcludeAddresses();
    void setupAddressesUseAllEnabled();
    void setupSplitter();
    void setupSplitterButtons();
    void updateGroup();
    void setupAddressGroup();

    const QList<AddressGroup *> &addressGroups() const;
    AddressGroup *addressGroupByIndex(int index) const;

    QString zonesText(bool include) const;

    static QString localNetworks();

private:
    int m_addressGroupIndex = -1;

    QTabBar *m_tabBar = nullptr;
    AddressesColumn *m_includeAddresses = nullptr;
    AddressesColumn *m_excludeAddresses = nullptr;
    TextArea2Splitter *m_splitter = nullptr;
    QPushButton *m_btAddLocals = nullptr;
};

#endif // ADDRESSESPAGE_H
