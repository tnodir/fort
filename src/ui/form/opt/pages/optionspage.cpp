#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../conf/firewallconf.h"
#include "../../../driver/drivermanager.h"
#include "../../../translationmanager.h"
#include "../../../util/stringutil.h"
#include "../../controls/controlutil.h"
#include "../optionscontroller.h"

OptionsPage::OptionsPage(OptionsController *ctrl,
                         QWidget *parent) :
    BasePage(ctrl, parent)
{
    setupUi();
}

void OptionsPage::setIniEdited(bool v)
{
    if (m_iniEdited != v) {
        m_iniEdited = v;

        if (m_iniEdited) {
            ctrl()->setOthersEdited(true);
        }
    }
}

void OptionsPage::onEditResetted()
{
    setIniEdited(false);
}

void OptionsPage::onSaved()
{
    if (!iniEdited()) return;

    settings()->setStartWithWindows(m_cbStart->isChecked());
    settings()->setHotKeyEnabled(m_cbHotKeys->isChecked());

    const auto password = m_editPassword->text();
    settings()->setPasswordHash(StringUtil::cryptoHash(password));
    m_editPassword->setText(QString());
}

void OptionsPage::onRetranslateUi()
{
    m_cbStart->setText(tr("Start with Windows"));
    m_cbProvBoot->setText(tr("Stop traffic when Fort Firewall is not running"));
    m_cbFilterEnabled->setText(tr("Filter Enabled"));
    m_cbFilterLocals->setText(tr("Filter Local Addresses"));
    m_cbStopTraffic->setText(tr("Stop Traffic"));
    m_cbStopInetTraffic->setText(tr("Stop Internet Traffic"));
    m_cbHotKeys->setText(tr("Hot Keys"));

    m_cbPassword->setText(tr("Password:"));
    retranslateEditPassword();

    m_labelLanguage->setText(tr("Language:"));

    m_labelDriver->setText(tr("Driver:"));
    retranslateDriverMessage();

    m_installDriverButton->setText(tr("Install"));
    m_removeDriverButton->setText(tr("Remove"));
}

void OptionsPage::setupUi()
{
    auto rowLayout = new QHBoxLayout();

    // Column #1
    auto colLayout1 = new QVBoxLayout();
    colLayout1->setSpacing(10);
    rowLayout->addLayout(colLayout1);

    m_cbStart = ControlUtil::createCheckBox(settings()->startWithWindows(), [&](bool) {
        setIniEdited(true);
    });
    m_cbProvBoot = ControlUtil::createCheckBox(conf()->provBoot(), [&](bool checked) {
        conf()->setProvBoot(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbFilterEnabled = ControlUtil::createCheckBox(conf()->filterEnabled(), [&](bool checked) {
        conf()->setFilterEnabled(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbFilterLocals = ControlUtil::createCheckBox(conf()->filterLocals(), [&](bool checked) {
        conf()->setFilterLocals(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbStopTraffic = ControlUtil::createCheckBox(conf()->stopTraffic(), [&](bool checked) {
        conf()->setStopTraffic(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbStopInetTraffic = ControlUtil::createCheckBox(conf()->stopInetTraffic(), [&](bool checked) {
        conf()->setStopInetTraffic(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbHotKeys = ControlUtil::createCheckBox(settings()->hotKeyEnabled(), [&](bool) {
        setIniEdited(true);
    });

    colLayout1->addWidget(m_cbStart);
    colLayout1->addWidget(m_cbProvBoot);
    colLayout1->addWidget(m_cbFilterEnabled);
    colLayout1->addWidget(m_cbFilterLocals);
    colLayout1->addWidget(m_cbStopTraffic);
    colLayout1->addWidget(m_cbStopInetTraffic);
    colLayout1->addWidget(m_cbHotKeys);

    // Password Row
    auto passwordLayout = new QHBoxLayout();
    passwordLayout->setSpacing(6);

    m_cbPassword = ControlUtil::createCheckBox(settings()->hasPassword(), [&](bool checked) {
        if (!checked) {
            m_editPassword->setText(QString());
        } else {
            m_editPassword->setFocus();
        }

        setIniEdited(true);
    });

    setupEditPassword();

    passwordLayout->addWidget(m_cbPassword);
    passwordLayout->addWidget(m_editPassword);
    passwordLayout->addStretch(1);

    colLayout1->addLayout(passwordLayout);

    // Language Row
    auto langLayout = new QHBoxLayout();
    langLayout->setSpacing(10);

    m_labelLanguage = new QLabel();

    setupComboLanguage();

    langLayout->addWidget(m_labelLanguage);
    langLayout->addWidget(m_comboLanguage);
    langLayout->addStretch(1);

    colLayout1->addLayout(langLayout);

    colLayout1->addStretch(1);

    // Column #2
    auto colLayout2 = new QVBoxLayout();
    colLayout2->setSpacing(10);
    rowLayout->addLayout(colLayout2, 1);

    auto driverFrame = setupDriverFrame();

    colLayout2->addWidget(driverFrame, 0, Qt::AlignHCenter);
    colLayout2->addStretch(1);

    this->setLayout(rowLayout);
}

void OptionsPage::setupEditPassword()
{
    m_editPassword = new QLineEdit();
    m_editPassword->setFixedWidth(200);
    m_editPassword->setEchoMode(QLineEdit::Password);

    const auto refreshEditPassword = [&] {
        m_editPassword->setReadOnly(settings()->hasPassword()
                                    || !m_cbPassword->isChecked());

        retranslateEditPassword();
    };

    refreshEditPassword();

    connect(settings(), &FortSettings::iniChanged, this, refreshEditPassword);
    connect(m_cbPassword, &QCheckBox::toggled, this, refreshEditPassword);
}

void OptionsPage::retranslateEditPassword()
{
    m_editPassword->setPlaceholderText(
                settings()->hasPassword() ? tr("Installed")
                                          : tr("Not Installed"));
}

void OptionsPage::setupComboLanguage()
{
    m_comboLanguage = ControlUtil::createComboBox(translationManager()->naturalLabels(), [&](int index) {
        if (translationManager()->switchLanguage(index)) {
            settings()->setLanguage(translationManager()->localeName());
        }
    });
    m_comboLanguage->setFixedWidth(200);

    const auto refreshComboLanguage = [&] {
        m_comboLanguage->setCurrentIndex(translationManager()->language());
    };

    refreshComboLanguage();

    connect(translationManager(), &TranslationManager::languageChanged, this, refreshComboLanguage);
}

QFrame *OptionsPage::setupDriverFrame()
{
    auto frame = new QFrame();
    frame->setFrameShape(QFrame::Panel);

    auto colLayout = new QVBoxLayout();
    colLayout->setMargin(15);
    colLayout->setSpacing(10);
    frame->setLayout(colLayout);

    // Label Row
    auto labelLayout = new QHBoxLayout();
    labelLayout->setSpacing(4);
    colLayout->addLayout(labelLayout);

    m_labelDriver = new QLabel();
    {
        QFont font;
        font.setBold(true);
        m_labelDriver->setFont(font);
    }

    m_labelDriverMessage = new QLabel();

    setupDriverIcon();

    labelLayout->addStretch(1);
    labelLayout->addWidget(m_iconDriver, 0, Qt::AlignTop);
    labelLayout->addWidget(m_labelDriver);
    labelLayout->addWidget(m_labelDriverMessage);
    labelLayout->addStretch(1);

    // Buttons Row
    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);
    colLayout->addLayout(buttonsLayout);

    m_installDriverButton = ControlUtil::createButton(QString(), [&] {
        if (fortManager()->showQuestionBox(tr("Install Driver?"))) {
            fortManager()->installDriver();
        }
    });
    m_removeDriverButton = ControlUtil::createButton(QString(), [&] {
        if (fortManager()->showQuestionBox(tr("Remove Driver?"))) {
            fortManager()->removeDriver();
        }
    });

    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(m_installDriverButton);
    buttonsLayout->addWidget(m_removeDriverButton);
    buttonsLayout->addStretch(1);

    return frame;
}

void OptionsPage::setupDriverIcon()
{
    m_iconDriver = new QLabel();

    const auto refreshDriverIcon = [&] {
        const auto iconPath = driverManager()->isDeviceOpened()
                ? (driverManager()->errorMessage().isEmpty()
                   ? ":/images/plugin.png"
                   : ":/images/plugin_error.png")
                : ":/images/plugin_disabled.png";

        m_iconDriver->setPixmap(QPixmap(iconPath));

        retranslateDriverMessage();
    };

    refreshDriverIcon();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverIcon);
}

void OptionsPage::retranslateDriverMessage()
{
    const auto text = driverManager()->isDeviceOpened()
            ? (!driverManager()->errorMessage().isEmpty()
               ? driverManager()->errorMessage()
               : tr("Installed"))
            : tr("Not Installed");

    m_labelDriverMessage->setText(text);
}
