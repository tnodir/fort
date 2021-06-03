#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "../../../conf/confmanager.h"
#include "../../../conf/firewallconf.h"
#include "../../../driver/drivermanager.h"
#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../manager/translationmanager.h"
#include "../../../manager/windowmanager.h"
#include "../../../task/taskinfoupdatechecker.h"
#include "../../../task/taskmanager.h"
#include "../../../user/iniuser.h"
#include "../../../util/iconcache.h"
#include "../../../util/osutil.h"
#include "../../../util/startuputil.h"
#include "../../controls/controlutil.h"
#include "../../dialog/passworddialog.h"
#include "../optionscontroller.h"

namespace {

struct Startup
{
    Startup() : initialized(false), isServiceChanged(false), wasService(false), isService(false) { }

    quint8 initialized : 1;
    quint8 isServiceChanged : 1;
    quint8 wasService : 1;
    quint8 isService : 1;
} g_startup;

void moveProfile(const QString &profilePath, const QString &newProfilePath)
{
    if (profilePath.isEmpty() || newProfilePath.isEmpty())
        return;

    if (QMessageBox::question(nullptr, OptionsPage::tr("Move Profile"),
                OptionsPage::tr("New profile path is \"%1\".\n"
                                "Would you like to move profile from \"%2\" to new location?")
                        .arg(newProfilePath, profilePath))
            != QMessageBox::Yes)
        return;

    QDir(newProfilePath).removeRecursively();
    QDir().rename(profilePath, newProfilePath);
}

}

OptionsPage::OptionsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupStartup();
    setupUi();
}

void OptionsPage::onAboutToSave()
{
    // Startup
    saveAutoRunMode(m_comboAutoRun->currentIndex());
    saveService(m_cbService->isChecked());

    // Password
    if (!settings()->hasPassword() && ini()->hasPassword() && ini()->password().isEmpty()) {
        m_cbPassword->setChecked(false);
    }
}

void OptionsPage::saveAutoRunMode(int mode)
{
    if (m_currentAutoRunMode == mode)
        return;

    m_currentAutoRunMode = mode;

    StartupUtil::setAutoRunMode(mode, settings()->defaultLanguage());
}

void OptionsPage::saveService(bool isService)
{
    if (g_startup.isService == isService)
        return;

    g_startup.isService = isService;

    if (!g_startup.isServiceChanged) {
        g_startup.isServiceChanged = true;

        const bool isDefaultProfilePath = settings()->isDefaultProfilePath();
        const QString profilePath = isDefaultProfilePath ? settings()->profilePath() : QString();
        const QString newProfilePath = isDefaultProfilePath
                ? FortSettings::defaultProfilePath(g_startup.isService)
                : QString();

        connect(fortManager(), &QObject::destroyed, [=] {
            if (g_startup.wasService == g_startup.isService)
                return;

            StartupUtil::setServiceInstalled(false);

            if (profilePath != newProfilePath) {
                moveProfile(profilePath, newProfilePath);
            }

            if (g_startup.isService) {
                StartupUtil::setServiceInstalled(true);
            }
        });
    }

    QMetaObject::invokeMethod(
            fortManager(), &FortManager::processRestartRequired, Qt::QueuedConnection);
}

void OptionsPage::onEditResetted()
{
    retranslateEditPassword();
}

void OptionsPage::onRetranslateUi()
{
    m_gbStartup->setTitle(tr("Startup"));
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbGlobal->setTitle(tr("Global"));
    m_gbLogs->setTitle(tr("Logs"));
    m_gbDriver->setTitle(tr("Driver"));
    m_gbNewVersion->setTitle(tr("New Version"));

    m_labelStartMode->setText(tr("Auto-run:"));
    retranslateComboStartMode();

    m_cbService->setText(tr("Run Fort Firewall as a Service in background"));
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
    m_btInstallDriver->setText(tr("Reinstall"));
    m_btRemoveDriver->setText(tr("Remove"));

    m_btNewVersion->setText(tr("Download"));
}

void OptionsPage::retranslateComboStartMode()
{
    const QStringList list = { tr("Disabled"), tr("For current user"), tr("For all users") };

    int currentIndex = m_comboAutoRun->currentIndex();
    if (m_comboAutoRun->currentIndex() < 0) {
        currentIndex = m_currentAutoRunMode;
    }

    m_comboAutoRun->clear();
    m_comboAutoRun->addItems(list);

    m_comboAutoRun->setCurrentIndex(currentIndex);

    // Disable some items if user is not an administrator
    if (settings()->isUserAdmin())
        return;

    m_cbService->setEnabled(false);

    if (currentIndex >= StartupUtil::StartupAllUsers) {
        m_comboAutoRun->setEnabled(false);
        return;
    }

    auto comboModel = qobject_cast<QStandardItemModel *>(m_comboAutoRun->model());
    if (!comboModel)
        return;

    const int itemCount = comboModel->rowCount();
    for (int i = StartupUtil::StartupAllUsers; i < itemCount; ++i) {
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

void OptionsPage::setupStartup()
{
    m_currentAutoRunMode = StartupUtil::autoRunMode();

    if (g_startup.initialized)
        return;

    g_startup.initialized = true;
    g_startup.wasService = g_startup.isService = settings()->hasService();
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

    m_cbService = ControlUtil::createCheckBox(
            g_startup.isService, [&](bool /*checked*/) { ctrl()->emitEdited(); });

    m_cbProvBoot = ControlUtil::createCheckBox(conf()->provBoot(), [&](bool checked) {
        conf()->setProvBoot(checked);
        ctrl()->setFlagsEdited();
    });

    auto layout = new QVBoxLayout();
    layout->addLayout(startModeLayout);
    layout->addWidget(m_cbService);
    layout->addWidget(m_cbProvBoot);

    m_gbStartup = new QGroupBox(this);
    m_gbStartup->setLayout(layout);
}

QLayout *OptionsPage::setupStartModeLayout()
{
    m_labelStartMode = ControlUtil::createLabel();

    m_comboAutoRun = ControlUtil::createComboBox(QStringList(), [&](int index) {
        if (m_currentAutoRunMode != index) {
            ctrl()->emitEdited();
        }
    });
    m_comboAutoRun->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelStartMode, m_comboAutoRun);
}

void OptionsPage::setupTrafficBox()
{
    m_cbFilterEnabled = ControlUtil::createCheckBox(conf()->filterEnabled(), [&](bool checked) {
        conf()->setFilterEnabled(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbFilterLocals = ControlUtil::createCheckBox(conf()->filterLocals(), [&](bool checked) {
        conf()->setFilterLocals(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbStopTraffic = ControlUtil::createCheckBox(conf()->stopTraffic(), [&](bool checked) {
        conf()->setStopTraffic(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbStopInetTraffic = ControlUtil::createCheckBox(conf()->stopInetTraffic(), [&](bool checked) {
        conf()->setStopInetTraffic(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbAllowAllNew = ControlUtil::createCheckBox(conf()->allowAllNew(), [&](bool checked) {
        conf()->setAllowAllNew(checked);
        ctrl()->setFlagsEdited();
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
    m_cbExplorerMenu = ControlUtil::createCheckBox(ini()->explorerIntegrated(), [&](bool checked) {
        ini()->setExplorerIntegrated(checked);
        ctrl()->setIniEdited();
    });
    m_cbExplorerMenu->setEnabled(settings()->hasService() || settings()->isUserAdmin());

    m_cbHotKeys = ControlUtil::createCheckBox(iniUser()->hotKeyEnabled(), [&](bool checked) {
        iniUser()->setHotKeyEnabled(checked);
        confManager()->saveIniUser(true);
    });

    // Password Row
    auto passwordLayout = setupPasswordLayout();
    setupPasswordLock();

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

    m_cbPassword = ControlUtil::createCheckBox(ini()->hasPassword(), [&](bool checked) {
        if (checked) {
            m_editPassword->setFocus();
        } else {
            m_editPassword->clear();
        }

        ini()->setHasPassword(checked);
        ctrl()->setIniEdited();
    });

    setupEditPassword();

    layout->addWidget(m_cbPassword);
    layout->addWidget(m_editPassword);

    return layout;
}

void OptionsPage::setupEditPassword()
{
    m_editPassword = ControlUtil::createLineEdit(QString(), [&](const QString &text) {
        m_cbPassword->setChecked(!text.isEmpty());

        ini()->setPassword(text);
        ctrl()->setIniEdited();
    });
    m_editPassword->setClearButtonEnabled(true);
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setMaxLength(32);
    m_editPassword->setFixedWidth(200);
}

void OptionsPage::setupPasswordLock()
{
    m_btPasswordLock = ControlUtil::createFlatButton(":/icons/lock-open.png", [&] {
        settings()->resetCheckedPassword();
        m_btPasswordLock->hide();
    });

    const auto refreshPasswordLock = [&] {
        m_btPasswordLock->setVisible(
                settings()->hasPassword() && !settings()->isPasswordRequired());
    };

    refreshPasswordLock();

    connect(settings(), &FortSettings::passwordCheckedChanged, this, refreshPasswordLock);
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
                    iniUser()->setLanguage(translationManager()->localeName());
                    confManager()->saveIniUser();
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
    m_cbLogDebug = ControlUtil::createCheckBox(ini()->logDebug(), [&](bool checked) {
        ini()->setLogDebug(checked);
        ctrl()->setIniEdited();
    });
    m_cbLogConsole = ControlUtil::createCheckBox(ini()->logConsole(), [&](bool checked) {
        ini()->setLogConsole(checked);
        ctrl()->setIniEdited();
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

    // Driver Group Box
    setupDriverBox();
    layout->addWidget(m_gbDriver);

    // New Version Group Box
    setupNewVersionBox();
    setupNewVersionUpdate();
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
        if (windowManager()->showQuestionBox(tr("Are you sure to reinstall the Driver?"))) {
            fortManager()->installDriver();
        }
    });
    m_btRemoveDriver = ControlUtil::createButton(QString(), [&] {
        if (windowManager()->showQuestionBox(tr("Are you sure to remove the Driver?"))) {
            fortManager()->removeDriver();
        }
    });

    if (!settings()->isUserAdmin()) {
        m_btInstallDriver->setEnabled(false);
        m_btRemoveDriver->setEnabled(false);
    }
    m_btRemoveDriver->setVisible(!settings()->hasService());

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

    const auto refreshDriverInfo = [&] {
        m_iconDriver->setEnabled(driverManager()->isDeviceOpened());
        retranslateDriverMessage();
    };

    refreshDriverInfo();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverInfo);
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
