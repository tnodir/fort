#ifndef ADDRESSESCOLUMN_H
#define ADDRESSESCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

class PlainTextEdit;
class ZonesSelector;

class AddressesColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AddressesColumn(QWidget *parent = nullptr);

    QLabel *labelTitle() const { return m_labelTitle; }
    QCheckBox *cbUseAll() const { return m_cbUseAll; }
    ZonesSelector *btSelectZones() const { return m_btSelectZones; }
    PlainTextEdit *editIpText() const { return m_editIpText; }

public slots:
    void retranslateUi();

private:
    void setupUi();
    QLayout *setupHeaderLayout();

private:
    QLabel *m_labelTitle = nullptr;
    QCheckBox *m_cbUseAll = nullptr;
    ZonesSelector *m_btSelectZones = nullptr;
    PlainTextEdit *m_editIpText = nullptr;
};

#endif // ADDRESSESCOLUMN_H
