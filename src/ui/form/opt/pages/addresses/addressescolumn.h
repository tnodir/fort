#ifndef ADDRESSESCOLUMN_H
#define ADDRESSESCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class PlainTextEdit;

class AddressesColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AddressesColumn(QWidget *parent = nullptr);

    qint8 zonesCount() const { return m_zonesCount; }
    void setZonesCount(qint8 v);

    QLabel *labelTitle() const { return m_labelTitle; }
    QCheckBox *cbUseAll() const { return m_cbUseAll; }
    QPushButton *btSelectZones() const { return m_btSelectZones; }
    PlainTextEdit *editIpText() const { return m_editIpText; }

public slots:
    void retranslateUi();

private:
    void retranslateZonesText();

    void setupUi();
    QLayout *setupZonesRow();

private:
    qint8 m_zonesCount = 0;

    QLabel *m_labelTitle = nullptr;
    QCheckBox *m_cbUseAll = nullptr;
    QToolButton *m_btOpenZones = nullptr;
    QPushButton *m_btSelectZones = nullptr;
    PlainTextEdit *m_editIpText = nullptr;
};

#endif // ADDRESSESCOLUMN_H
