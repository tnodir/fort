#include "passworddialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <form/controls/controlutil.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/guiutil.h>
#include <util/window/widgetwindow.h>

PasswordDialog::PasswordDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    retranslateUi();
}

QString PasswordDialog::password() const
{
    return m_editPassword->text();
}

int PasswordDialog::unlockType() const
{
    return m_comboUnlock->currentIndex();
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
    ControlUtil::setComboBoxTexts(
            m_comboUnlock, FortSettings::unlockTypeStrings(), FortSettings::UnlockWindow);
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

    // Focus
    m_editPassword->setFocus();

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/key.png"));
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

    m_comboUnlock = ControlUtil::createComboBox();
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
