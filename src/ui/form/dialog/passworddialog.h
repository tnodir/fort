#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = nullptr);

    QString password() const;
    int unlockType() const;

private:
    void retranslateUi();
    void retranslateComboUnlock();

    void setupUi();
    QLayout *setupPasswordLayout();
    QLayout *setupUnlockLayout();
    void setupButtonBox();

private:
    QLabel *m_labelPassword = nullptr;
    QLineEdit *m_editPassword = nullptr;
    QLabel *m_labelUnlock = nullptr;
    QComboBox *m_comboUnlock = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};

#endif // PASSWORDDIALOG_H
