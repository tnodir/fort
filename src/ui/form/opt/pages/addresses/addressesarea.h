#ifndef ADDRESSESAREA_H
#define ADDRESSESAREA_H

#include <QWidget>

#include <conf/firewallconf.h>

QT_FORWARD_DECLARE_CLASS(QLabel)

class PlainTextEdit;
class ZonesSelector;

class AddressesArea : public QWidget
{
    Q_OBJECT

public:
    explicit AddressesArea(FirewallConf::AddressList addressList, const QString &iconPath,
            QWidget *parent = nullptr);

    FirewallConf::AddressList addressList() const { return m_addressList; }

    PlainTextEdit *editIpText() const { return m_editIpText; }

    void setZones(quint32 zones);
    void setText(const QString &text);

signals:
    void zonesEdited(quint32 zones);
    void textEdited(const QString &text);

public slots:
    void retranslateUi();

private:
    void setupUi(const QString &iconPath);
    QLayout *setupHeaderLayout(const QString &iconPath);
    void setupEditIpText();

private:
    FirewallConf::AddressList m_addressList = FirewallConf::AddressListInet;

    QLabel *m_labelTitle = nullptr;
    ZonesSelector *m_btSelectZones = nullptr;
    PlainTextEdit *m_editIpText = nullptr;
};

#endif // ADDRESSESAREA_H
