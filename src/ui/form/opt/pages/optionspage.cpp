#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../driver/drivermanager.h"
#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../task/taskinfoupdatechecker.h"
#include "../../../task/taskmanager.h"
#include "../../../translationmanager.h"
#include "../../../util/iconcache.h"
#include "../../../util/osutil.h"
#include "../../../util/startuputil.h"
#include "../../../util/stringutil.h"
#include "../../controls/controlutil.h"
#include "../../dialog/passworddialog.h"
#include "../optionscontroller.h"

OptionsPage::OptionsPage(OptionsController *ctrl, QWidget *parent) :
    BasePage(ctrl, parent), m_iniEdited(false), m_currentStartMode(0), m_explorerIntegrated(false)
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
    saveStartMode();
    saveExplorerIntegration();

    if (iniEdited()) {
        saveIni();
    }
}

void OptionsPage::saveStartMode()
{
    if (m_currentStartMode != m_comboStartMode->currentIndex()) {
        const bool wasServiceMode = StartupUtil::isServiceMode(m_currentStartMode);

        m_currentStartMode = m_comboStartMode->currentIndex();
        StartupUtil::setStartupMode(m_currentStartMode, settings()->defaultLanguage());

        const bool isServiceMode = StartupUtil::isServiceMode(m_currentStartMode);
        if (isServiceMode != wasServiceMode) {
            QMetaObject::invokeMethod(
                    fortManager(), &FortManager::processRestartRequired, Qt::QueuedConnection);
        }
    }
}

void OptionsPage::saveExplorerIntegration()
{
    if (m_explorerIntegrated != m_cbExplorerMenu->isChecked()) {
        m_explorerIntegrated = m_cbExplorerMenu->isChecked();
        StartupUtil::integrateExplorer(m_explorerIntegrated);
    }
}

void OptionsPage::saveIni()
{
    if (m_cbPassword->isChecked() != settings()->hasPassword()) {
        const auto password = m_editPassword->text();
        settings()->setPasswordHash(StringUtil::cryptoHash(password));
        m_editPassword->clear();

        if (password.isEmpty() && m_btPasswordLock->isVisible()) {
            m_btPasswordLock->click(); // Reset unlocked password
        }
    }
}

void OptionsPage::onRetranslateUi()
{
    m_gbStartup->setTitle(tr("Startup"));
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbGlobal->setTitle(tr("Global"));
    m_gbLogs->setTitle(tr("Logs"));
    m_gbDriver->setTitle(tr("Driver"));
    m_gbNewVersion->setTitle(tr("New Version"));

    m_labelStartMode->setText(tr("Startup mode:"));
    retranslateComboStartMode();

    m_cbProvBoot->setText(tr("Stop traffic when Fort Firewall is not running"));
    m_cbFilterEnabled->setText(tr("Filter Enabled"));
    m_cbFilterLocals->setText(tr("Filter Local Addresses"));
    m_cbFilterLocals->setToolTip(
            tr("Filter Local Loopback (127.0.0.0/8) and Broadcast (255.255.255.255) Addresses"));
    m_cbStopTraffic->setText(tr("Stop Traffic"));
    m_cbStopInetTraffic->setText(tr("Stop Internet Traffic"));
    m_cbAllowAllNew->setText(tr("Auto-Allow New Programs"));

    m_cbExplorerMenu->setText(tr("Windows Explorer integration"));
    m_cbHotKeys->setText(tr("Hot Keys"));

    m_cbPassword->setText(tr("Password:"));
    retranslateEditPassword();
    m_btPasswordLock->setText(
            tr("Lock the password (unlocked till \"%1\")")
                    .arg(PasswordDialog::unlockTypeStrings().at(settings()->passwordUnlockType())));

    m_labelLanguage->setText(tr("Language:"));

    m_cbLogDebug->setText(tr("Log debug messages"));
    m_cbLogConsole->setText(tr("Show log messages in console"));

    retranslateDriverMessage();
    m_btInstallDriver->setText(tr("Install"));
    m_btRemoveDriver->setText(tr("Remove"));

    m_btNewVersion->setText(tr("Download"));
}

void OptionsPage::retranslateComboStartMode()
{
    const QStringList list = {
        tr("Disabled"), tr("For current user")
        // TODO: Implement service RPC
        //, tr("For all users"), tr("For all users in background")
    };

    int currentIndex = m_comboStartMode->currentIndex();
    if (m_comboStartMode->currentIndex() < 0) {
        currentIndex = m_currentStartMode = StartupUtil::getStartupMode();
    }

    m_comboStartMode->clear();
    m_comboStartMode->addItems(list);

    m_comboStartMode->setCurrentIndex(currentIndex);

    // Disable some items if user is not an administrator
    if (OsUtil::isUserAdmin())
        return;

    auto comboModel = qobject_cast<QStandardItemModel *>(m_comboStartMode->model());
    if (!comboModel)
        return;

    const int itemCount = comboModel->rowCount();
    const bool isServiceMode = StartupUtil::isServiceMode(currentIndex);
    for (int i = (isServiceMode ? StartupUtil::StartupDisabled : StartupUtil::StartupAllUsers);
            i < itemCount; ++i) {
        auto item = comboModel->item(i);
        if (item) {
            item->setEnabled(false);
        }
    }
}

void OptionsPage::retranslateEditPassword()
{
    m_editPassword->setPlaceholderText(
            settings()->hasPassword() ? tr("Installed") : tr("Not Installed"));
}

void OptionsPage::retranslateDriverMessage()
{
    const auto text = driverManager()->isDeviceError()
            ? driverManager()->errorMessage()
            : (driverManager()->isDeviceOpened() ? tr("Installed") : tr("Not Installed"));

    m_labelDriverMessage->setText(text);
}

void OptionsPage::setupUi()
{
    // Column #1
    auto colLayout1 = setupColumn1();

    // Column #2
    auto colLayout2 = setupColumn2();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();

    this->setLayout(layout);
}

QLayout *OptionsPage::setupColumn1()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    // Startup Group Box
    setupStartupBox();
    layout->addWidget(m_gbStartup);

    // Traffic Group Box
    setupTrafficBox();
    layout->addWidget(m_gbTraffic);

    // Global Group Box
    setupGlobalBox();
    layout->addWidget(m_gbGlobal);

    // Logs Group Box
    setupLogsBox();
    layout->addWidget(m_gbLogs);

    layout->addStretch();

    return layout;
}

void OptionsPage::setupStartupBox()
{
    auto startModeLayout = setupStartModeLayout();

    m_cbProvBoot = ControlUtil::createCheckBox(conf()->provBoot(), [&](bool checked) {
        conf()->setProvBoot(checked);
        ctrl()->setConfFlagsEdited(true);
    });

    auto layout = new QVBoxLayout();
    layout->addLayout(startModeLayout);
    layout->addWidget(m_cbProvBoot);

    m_gbStartup = new QGroupBox(this);
    m_gbStartup->setLayout(layout);
}

QLayout *OptionsPage::setupStartModeLayout()
{
    m_labelStartMode = ControlUtil::createLabel();

    m_comboStartMode =
            ControlUtil::createComboBox(QStringList(), [&](int) { ctrl()->setOthersEdited(true); });
    m_comboStartMode->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelStartMode, m_comboStartMode);
}

void OptionsPage::setupTrafficBox()
{
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
    m_cbAllowAllNew = ControlUtil::createCheckBox(conf()->allowAllNew(), [&](bool checked) {
        conf()->setAllowAllNew(checked);
        ctrl()->setConfFlagsEdited(true);
    });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addWidget(m_cbFilterLocals);
    layout->addWidget(m_cbStopTraffic);
    layout->addWidget(m_cbStopInetTraffic);
    layout->addWidget(m_cbAllowAllNew);

    m_gbTraffic = new QGroupBox(this);
    m_gbTraffic->setLayout(layout);
}

void OptionsPage::setupGlobalBox()
{
    m_explorerIntegrated = StartupUtil::isExplorerIntegrated();
    m_cbExplorerMenu = ControlUtil::createCheckBox(
            m_explorerIntegrated, [&](int) { ctrl()->setOthersEdited(true); });
    m_cbExplorerMenu->setEnabled(settings()->hasService() || OsUtil::isUserAdmin());

    m_cbHotKeys = ControlUtil::createCheckBox(conf()->hotKeyEnabled(), [&](bool checked) {
        conf()->setHotKeyEnabled(checked);
        ctrl()->setConfFlagsEdited(true);
    });

    // Password Row
    auto passwordLayout = setupPasswordLayout();

    // Language Row
    auto langLayout = setupLangLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbExplorerMenu);
    layout->addWidget(m_cbHotKeys);
    layout->addLayout(passwordLayout);
    layout->addWidget(m_btPasswordLock, 0, Qt::AlignCenter);
    layout->addLayout(langLayout);

    m_gbGlobal = new QGroupBox(this);
    m_gbGlobal->setLayout(layout);
}

QLayout *OptionsPage::setupPasswordLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(6);

    m_cbPassword = ControlUtil::createCheckBox(settings()->hasPassword(), [&](bool checked) {
        m_editPassword->clear();
        if (checked) {
            m_editPassword->setFocus();
        }

        setIniEdited(true);
    });

    m_btPasswordLock = ControlUtil::createFlatButton(":/icons/lock-open.png", [&] {
        settings()->resetCheckedPassword();
        m_btPasswordLock->hide();
    });

    setupEditPassword();

    layout->addWidget(m_cbPassword);
    layout->addWidget(m_editPassword);

    return layout;
}

void OptionsPage::setupEditPassword()
{
    m_editPassword = new QLineEdit();
    m_editPassword->setClearButtonEnabled(true);
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setFixedWidth(200);

    const auto refreshEditPassword = [&] {
        const bool hasPassword = settings()->hasPassword();

        m_editPassword->setReadOnly(hasPassword || !m_cbPassword->isChecked());
        retranslateEditPassword();

        m_btPasswordLock->setVisible(hasPassword && !settings()->isPasswordRequired());
    };

    refreshEditPassword();

    connect(settings(), &FortSettings::passwordStateChanged, this, refreshEditPassword);
    connect(m_cbPassword, &QCheckBox::toggled, this, refreshEditPassword);
}

QLayout *OptionsPage::setupLangLayout()
{
    m_labelLanguage = ControlUtil::createLabel();

    setupComboLanguage();

    return ControlUtil::createRowLayout(m_labelLanguage, m_comboLanguage);
}

void OptionsPage::setupComboLanguage()
{
    m_comboLanguage =
            ControlUtil::createComboBox(translationManager()->naturalLabels(), [&](int index) {
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

void OptionsPage::setupLogsBox()
{
    m_cbLogDebug = ControlUtil::createCheckBox(conf()->logDebug(), [&](bool checked) {
        conf()->setLogDebug(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbLogConsole = ControlUtil::createCheckBox(conf()->logConsole(), [&](bool checked) {
        conf()->setLogConsole(checked);
        ctrl()->setConfFlagsEdited(true);
    });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbLogDebug);
    layout->addWidget(m_cbLogConsole);

    m_gbLogs = new QGroupBox(this);
    m_gbLogs->setLayout(layout);
}

QLayout *OptionsPage::setupColumn2()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    setupDriverBox();

    setupNewVersionBox();
    setupNewVersionUpdate();

    layout->addWidget(m_gbDriver);
    layout->addWidget(m_gbNewVersion);
    layout->addStretch();

    return layout;
}

void OptionsPage::setupDriverBox()
{
    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);

    // Label Row
    auto labelLayout = new QHBoxLayout();
    labelLayout->setSpacing(4);
    colLayout->addLayout(labelLayout);

    m_labelDriverMessage = ControlUtil::createLabel();
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

    m_gbDriver = new QGroupBox(this);
    m_gbDriver->setLayout(colLayout);
}

void OptionsPage::setupDriverIcon()
{
    m_iconDriver = ControlUtil::createLabel();
    m_iconDriver->setScaledContents(true);
    m_iconDriver->setMaximumSize(16, 16);
    m_iconDriver->setPixmap(IconCache::file(":/icons/puzzle.png"));

    const auto refreshDriverIcon = [&] {
        m_iconDriver->setEnabled(driverManager()->isDeviceOpened());
    };

    refreshDriverIcon();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverIcon);
    connect(driverManager(), &DriverManager::errorCodeChanged, this,
            &OptionsPage::retranslateDriverMessage);
}

void OptionsPage::setupNewVersionBox()
{
    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);

    // Label
    m_labelNewVersion = ControlUtil::createLabel();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    m_labelNewVersion->setTextFormat(Qt::MarkdownText);
#endif
    m_labelNewVersion->setWordWrap(true);
    m_labelNewVersion->setOpenExternalLinks(true);
    colLayout->addWidget(m_labelNewVersion);

    // Button
    m_btNewVersion = ControlUtil::createLinkButton(":/icons/download.png");

    connect(m_btNewVersion, &QAbstractButton::clicked, this, &OptionsPage::onLinkClicked);

    colLayout->addWidget(m_btNewVersion, 0, Qt::AlignHCenter);

    m_gbNewVersion = new QGroupBox(this);
    m_gbNewVersion->setMinimumWidth(380);
    m_gbNewVersion->setLayout(colLayout);
}

void OptionsPage::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto updateChecker = taskManager()->taskInfoUpdateChecker();
        m_gbNewVersion->setVisible(updateChecker->isNewVersion());
        m_labelNewVersion->setText(updateChecker->releaseText());
        m_btNewVersion->setWindowFilePath(updateChecker->downloadUrl());
        m_btNewVersion->setToolTip(updateChecker->downloadUrl());
    };

    refreshNewVersion();

    connect(taskManager()->taskInfoUpdateChecker(), &TaskInfoUpdateChecker::versionChanged, this,
            refreshNewVersion);
}
