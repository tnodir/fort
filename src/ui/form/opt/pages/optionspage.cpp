#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../driver/drivermanager.h"
#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../task/taskinfoupdatechecker.h"
#include "../../../task/taskmanager.h"
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

        if (iniEdited()) {
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
    m_gbStartup->setTitle(tr("Startup"));
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbGlobal->setTitle(tr("Global"));
    m_gbDriver->setTitle(tr("Driver"));
    m_gbNewVersion->setTitle(tr("New Version"));

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

    retranslateDriverMessage();
    m_btInstallDriver->setText(tr("Install"));
    m_btRemoveDriver->setText(tr("Remove"));

    m_btNewVersion->setText(tr("Download"));
}

void OptionsPage::setupUi()
{
    // Column #1
    auto colLayout1 = new QVBoxLayout();
    colLayout1->setSpacing(10);

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

    m_gbStartup = new QGroupBox(this);
    auto startupLayout = new QVBoxLayout();
    startupLayout->addWidget(m_cbStart);
    startupLayout->addWidget(m_cbProvBoot);
    m_gbStartup->setLayout(startupLayout);
    colLayout1->addWidget(m_gbStartup);

    m_gbTraffic = new QGroupBox(this);
    auto trafficLayout = new QVBoxLayout();
    trafficLayout->addWidget(m_cbFilterEnabled);
    trafficLayout->addWidget(m_cbFilterLocals);
    trafficLayout->addWidget(m_cbStopTraffic);
    trafficLayout->addWidget(m_cbStopInetTraffic);
    m_gbTraffic->setLayout(trafficLayout);
    colLayout1->addWidget(m_gbTraffic);

    // Password Row
    auto passwordLayout = new QHBoxLayout();
    passwordLayout->setSpacing(6);

    m_cbPassword = ControlUtil::createCheckBox(settings()->hasPassword(), [&](bool checked) {
        if (checked) {
            m_editPassword->setFocus();
        } else {
            m_editPassword->setText(QString());
        }

        setIniEdited(true);
    });

    setupEditPassword();

    passwordLayout->addWidget(m_cbPassword);
    passwordLayout->addWidget(m_editPassword);

    // Language Row
    auto langLayout = new QHBoxLayout();
    langLayout->setSpacing(10);

    m_labelLanguage = new QLabel();

    setupComboLanguage();

    langLayout->addWidget(m_labelLanguage);
    langLayout->addWidget(m_comboLanguage);

    m_gbGlobal = new QGroupBox(this);
    auto globalLayout = new QVBoxLayout();
    globalLayout->addWidget(m_cbHotKeys);
    globalLayout->addLayout(passwordLayout);
    globalLayout->addLayout(langLayout);
    m_gbGlobal->setLayout(globalLayout);
    colLayout1->addWidget(m_gbGlobal);

    colLayout1->addStretch();

    // Column #2
    auto colLayout2 = new QVBoxLayout();
    colLayout2->setSpacing(10);

    setupDriverBox();

    setupNewVersionBox();
    setupNewVersionUpdate();

    colLayout2->addWidget(m_gbDriver);
    colLayout2->addWidget(m_gbNewVersion);
    colLayout2->addStretch();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();

    this->setLayout(layout);
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

void OptionsPage::setupDriverBox()
{
    m_gbDriver = new QGroupBox(this);

    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);
    m_gbDriver->setLayout(colLayout);

    // Label Row
    auto labelLayout = new QHBoxLayout();
    labelLayout->setSpacing(4);
    colLayout->addLayout(labelLayout);

    m_labelDriverMessage = new QLabel();
    m_labelDriverMessage->setWordWrap(true);
    m_labelDriverMessage->setFont(ControlUtil::fontDemiBold());

    setupDriverIcon();

    labelLayout->addStretch();
    labelLayout->addWidget(m_iconDriver, 0, Qt::AlignTop);
    labelLayout->addWidget(m_labelDriverMessage);
    labelLayout->addStretch();

    // Buttons Row
    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);
    colLayout->addLayout(buttonsLayout);

    m_btInstallDriver = ControlUtil::createButton(QString(), [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to install the Driver?"))) {
            fortManager()->installDriver();
        }
    });
    m_btRemoveDriver = ControlUtil::createButton(QString(), [&] {
        if (fortManager()->showQuestionBox(tr("Are you sure to remove the Driver?"))) {
            fortManager()->removeDriver();
        }
    });

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_btInstallDriver);
    buttonsLayout->addWidget(m_btRemoveDriver);
    buttonsLayout->addStretch();
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

void OptionsPage::setupNewVersionBox()
{
    m_gbNewVersion = new QGroupBox(this);

    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);
    m_gbNewVersion->setLayout(colLayout);

    // Label
    m_labelNewVersion = new QLabel();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    m_labelNewVersion->setTextFormat(Qt::MarkdownText);
#endif
    m_labelNewVersion->setWordWrap(true);
    m_labelNewVersion->setOpenExternalLinks(true);
    colLayout->addWidget(m_labelNewVersion, 0, Qt::AlignHCenter);

    // Button
    m_btNewVersion = ControlUtil::createLinkButton(":/images/server_compressed.png");

    connect(m_btNewVersion, &QAbstractButton::clicked, this, &OptionsPage::onLinkClicked);

    colLayout->addWidget(m_btNewVersion, 0, Qt::AlignHCenter);
}

void OptionsPage::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto updateChecker = taskManager()->taskInfoUpdateChecker();
        m_gbNewVersion->setVisible(!updateChecker->version().isEmpty());
        m_labelNewVersion->setText(updateChecker->releaseText());
        m_btNewVersion->setWindowFilePath(updateChecker->downloadUrl());
        m_btNewVersion->setToolTip(updateChecker->downloadUrl());
    };

    refreshNewVersion();

    connect(taskManager()->taskInfoUpdateChecker(), &TaskInfoUpdateChecker::versionChanged,
            this, refreshNewVersion);
}
