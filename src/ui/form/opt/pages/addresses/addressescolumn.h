#ifndef ADDRESSESCOLUMN_H
#define ADDRESSESCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

class AddressesColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AddressesColumn(QWidget *parent = nullptr);

    QLabel *labelTitle() const { return m_labelTitle; }
    QCheckBox *cbUseAll() const { return m_cbUseAll; }
    QPlainTextEdit *editIpText() const { return m_editIpText; }

private:
    void setupUi();

private:
    QLabel *m_labelTitle = nullptr;
    QCheckBox *m_cbUseAll = nullptr;
    QPlainTextEdit *m_editIpText = nullptr;
};

#endif // ADDRESSESCOLUMN_H
