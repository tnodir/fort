#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class PasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordDialog(QWidget *parent = nullptr);

    static QString getPassword(QWidget *parent = nullptr);

private:
    void retranslateUi();

    void setupUi();
    QLayout *setupPasswordLayout();
    void setupButtonBox();

private:
    QLabel *m_labelPassword = nullptr;
    QLineEdit *m_editPassword = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};

#endif // PASSWORDDIALOG_H
