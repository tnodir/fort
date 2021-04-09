#include "passworddialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "../controls/controlutil.h"
#include "../../util/guiutil.h"

PasswordDialog::PasswordDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    retranslateUi();
}

void PasswordDialog::retranslateUi()
{
    m_labelPassword->setText(tr("Please enter the password:"));

    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    this->setWindowTitle(tr("Password input"));
}

void PasswordDialog::setupUi()
{
    // Password Row
    auto passwordLayout = setupPasswordLayout();

    // Button Box
    setupButtonBox();

    // Main layout
    auto layout = new QVBoxLayout();
    layout->addLayout(passwordLayout);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_buttonBox);

    this->setLayout(layout);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png", ":/icons/key.png"));
}

QLayout *PasswordDialog::setupPasswordLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(6);

    m_labelPassword = ControlUtil::createLabel();

    m_editPassword = new QLineEdit();
    m_editPassword->setClearButtonEnabled(true);
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setMinimumWidth(150);

    layout->addWidget(m_labelPassword);
    layout->addWidget(m_editPassword);

    return layout;
}

void PasswordDialog::setupButtonBox()
{
    m_buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
    QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QString PasswordDialog::getPassword(QWidget *parent)
{
    PasswordDialog dialog(parent);
    return dialog.exec() == 1 ? dialog.m_editPassword->text() : QString();
}
