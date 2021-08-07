#include "passworddialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "../../manager/windowmanager.h"
#include "../../util/guiutil.h"
#include "../controls/controlutil.h"

PasswordDialog::PasswordDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    retranslateUi();
}

void PasswordDialog::retranslateUi()
{
    m_labelPassword->setText(tr("Please enter the password:"));

    m_labelUnlock->setText(tr("Unlock till:"));
    retranslateComboUnlock();

    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    this->setWindowTitle(tr("Password input"));
}

void PasswordDialog::retranslateComboUnlock()
{
    m_comboUnlock->clear();
    m_comboUnlock->addItems(unlockTypeStrings());
    m_comboUnlock->setCurrentIndex(0);
}

void PasswordDialog::setupUi()
{
    // Password Row
    auto passwordLayout = setupPasswordLayout();

    // Button Box
    setupButtonBox();

    // Unlock Row
    auto unlockLayout = setupUnlockLayout();

    // Main layout
    auto layout = new QVBoxLayout();
    layout->addLayout(passwordLayout);
    layout->addLayout(unlockLayout);
    layout->addStretch();
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_buttonBox);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/key.png"));
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

QLayout *PasswordDialog::setupUnlockLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(6);

    m_labelUnlock = ControlUtil::createLabel();

    m_comboUnlock = new QComboBox();
    m_comboUnlock->setMinimumWidth(150);

    layout->addWidget(m_labelUnlock);
    layout->addWidget(m_comboUnlock);

    return layout;
}

void PasswordDialog::setupButtonBox()
{
    m_buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
    QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

bool PasswordDialog::getPassword(QString &password, UnlockType &unlockType, QWidget *parent)
{
    PasswordDialog dialog(parent);

    WindowManager::showWidget(&dialog);

    if (dialog.exec() == 0)
        return false;

    password = dialog.m_editPassword->text();
    unlockType = static_cast<UnlockType>(dialog.m_comboUnlock->currentIndex());

    return true;
}

QStringList PasswordDialog::unlockTypeStrings()
{
    return { tr("Disabled"), tr("Session lockout"), tr("Program exit") };
}
