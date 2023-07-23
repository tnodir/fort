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
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>
#include <form/tray/trayicon.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/osutil.h>
#include <util/startuputil.h>

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

OptionsPage::OptionsPage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent), m_passwordEdited(false), m_languageEdited(false)
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
    if (passwordEdited()) {
        const bool isPasswordCleared = (ini()->hasPassword() && ini()->password().isEmpty());
        if (isPasswordCleared && !settings()->hasPassword()) {
            m_cbPassword->setChecked(false);
        }

        if (!ini()->hasPassword()) {
            settings()->resetCheckedPassword();
        }
    } else if (conf()->iniEdited()) {
        ini()->setHasPassword(settings()->hasPassword());
    }
}

void OptionsPage::onEditResetted()
{
    // Password
    setPasswordEdited(false);
    retranslateEditPassword();

    // Language
    if (languageEdited()) {
        setLanguageEdited(false);
        translationManager()->switchLanguageByName(confManager()->iniUser().language());
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

void OptionsPage::onRetranslateUi()
{
    m_gbStartup->setTitle(tr("Startup"));
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbProtection->setTitle(tr("Self Protection"));
    m_gbProg->setTitle(tr("Programs"));
    m_gbGlobal->setTitle(tr("Global"));
    m_gbHotKeys->setTitle(tr("Hot Keys"));
    m_gbHome->setTitle(tr("My Fort"));
    m_gbTray->setTitle(tr("Tray"));
    m_gbConfirmations->setTitle(tr("Action Confirmations"));
    m_gbLogs->setTitle(tr("Logs"));

    m_labelStartMode->setText(tr("Auto-run:"));
    retranslateComboStartMode();

    m_cbService->setText(tr("Run Fort Firewall as a Service in background"));

    m_cbFilterEnabled->setText(tr("Filter Enabled"));
    m_cbStopTraffic->setText(tr("Stop Traffic"));
    m_cbStopInetTraffic->setText(tr("Stop Internet Traffic"));

    m_labelFilterMode->setText(tr("Filter Mode:"));
    retranslateComboFilterMode();

    m_cbBootFilter->setText(tr("Stop traffic when Fort Firewall is not running"));
    m_cbFilterLocals->setText(tr("Filter Local Addresses"));
    m_cbFilterLocals->setToolTip(
            tr("Filter Local Loopback (127.0.0.0/8) and Broadcast (255.255.255.255) Addresses"));
    m_cbNoServiceControl->setText(tr("Disable Service controls"));
    m_cbCheckPasswordOnUninstall->setText(tr("Check password on Uninstall"));

    m_cbPassword->setText(tr("Password:"));
    retranslateEditPassword();

    m_btPasswordLock->setText(tr("Lock the password (unlocked till \"%1\")")
                                      .arg(settings()->passwordUnlockedTillText()));

    m_cbLogBlocked->setText(tr("Collect New Blocked Programs"));
    m_cbPurgeOnStart->setText(tr("Purge Obsolete on startup"));

    m_cbExplorerMenu->setText(tr("Windows Explorer integration"));
    m_cbUseSystemLocale->setText(tr("Use System Regional Settings"));
    m_labelLanguage->setText(tr("Language:"));

    m_cbHotKeysEnabled->setText(tr("Enabled"));
    m_cbHotKeysGlobal->setText(tr("Global"));

    m_cbHomeAutoShowMenu->setText(tr("Auto-Show Menu"));

    m_cbTrayShowIcon->setText(tr("Show Icon"));
    m_cbTrayAnimateAlert->setText(tr("Animate Alert Icon"));
    m_labelTrayEvent->setText(tr("Event:"));
    m_labelTrayAction->setText(tr("Action:"));
    retranslateComboTrayEvent();
    retranslateComboTrayAction();
    refreshComboTrayAction();

    m_cbConfirmTrayFlags->setText(tr("Tray Menu Flags"));
    m_cbConfirmQuit->setText(tr("Quit"));

    m_cbLogDebug->setText(tr("Log debug messages"));
    m_cbLogConsole->setText(tr("Show log messages in console"));
}

void OptionsPage::retranslateComboStartMode()
{
    const QStringList list = { tr("Disabled"), tr("For current user"), tr("For all users") };

    int currentIndex = m_comboAutoRun->currentIndex();
    if (currentIndex < 0) {
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

void OptionsPage::retranslateComboFilterMode()
{
    int index = 0;
    const QStringList iconPaths = FirewallConf::filterModeIconPaths();
    for (const QString &name : FirewallConf::filterModeNames()) {
        const QString iconPath = iconPaths.at(index);

        m_comboFilterMode->setItemText(index, name);
        m_comboFilterMode->setItemIcon(index, IconCache::icon(iconPath));

        ++index;
    }

    m_comboFilterMode->setCurrentIndex(conf()->filterModeIndex());
}

void OptionsPage::retranslateEditPassword()
{
    m_editPassword->setPlaceholderText(
            settings()->hasPassword() ? tr("Installed") : tr("Not Installed"));
}

void OptionsPage::retranslateComboTrayEvent()
{
    // Sync with TrayIcon::ClickType
    const QStringList list = { tr("Single Click"), tr("Ctrl + Single Click"),
        tr("Alt + Single Click"), tr("Double Click"), tr("Middle Click"), tr("Right Click") };

    int currentIndex = m_comboTrayEvent->currentIndex();
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    m_comboTrayEvent->clear();
    m_comboTrayEvent->addItems(list);

    m_comboTrayEvent->setCurrentIndex(currentIndex);
}

void OptionsPage::retranslateComboTrayAction()
{
    const TrayIcon::ActionType type = TrayIcon::ActionNone; // to find the enum usages
    Q_UNUSED(type);

    // Sync with TrayIcon::ActionType
    const QStringList list = { tr("Show My Fort"), tr("Show Programs"), tr("Show Options"),
        tr("Show Statistics"), tr("Show/Hide Traffic Graph"), tr("Switch Filter Enabled"),
        tr("Switch Stop Traffic"), tr("Switch Stop Internet Traffic"), tr("Show Filter Mode Menu"),
        tr("Show Tray Menu"), tr("Ignore") };

    m_comboTrayAction->clear();
    m_comboTrayAction->addItems(list);
    m_comboTrayAction->setCurrentIndex(-1);
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

    // Protection Group Box
    setupProtectionBox();
    layout->addWidget(m_gbProtection);

    // Programs Group Box
    setupProgBox();
    layout->addWidget(m_gbProg);

    layout->addStretch();

    return layout;
}

void OptionsPage::setupStartupBox()
{
    auto startModeLayout = setupStartModeLayout();

    m_cbService = ControlUtil::createCheckBox(
            g_startup.isService, [&](bool /*checked*/) { ctrl()->emitEdited(); });

    auto layout = new QVBoxLayout();
    layout->addLayout(startModeLayout);
    layout->addWidget(m_cbService);

    m_gbStartup = new QGroupBox();
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
    m_cbStopTraffic = ControlUtil::createCheckBox(conf()->stopTraffic(), [&](bool checked) {
        conf()->setStopTraffic(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbStopInetTraffic = ControlUtil::createCheckBox(conf()->stopInetTraffic(), [&](bool checked) {
        conf()->setStopInetTraffic(checked);
        ctrl()->setFlagsEdited();
    });

    auto filterModeLayout = setupFilterModeLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addWidget(m_cbStopTraffic);
    layout->addWidget(m_cbStopInetTraffic);
    layout->addLayout(filterModeLayout);

    m_gbTraffic = new QGroupBox();
    m_gbTraffic->setLayout(layout);
}

QLayout *OptionsPage::setupFilterModeLayout()
{
    m_labelFilterMode = ControlUtil::createLabel();

    m_comboFilterMode =
            ControlUtil::createComboBox(FirewallConf::filterModeNames(), [&](int index) {
                if (conf()->filterModeIndex() != index) {
                    conf()->setFilterModeIndex(index);
                    ctrl()->setFlagsEdited();
                }
            });
    m_comboFilterMode->setFixedWidth(200);

    // TODO: Implement Ask to Connect
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_comboFilterMode->model());
    QStandardItem *item = model->item(1);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    return ControlUtil::createRowLayout(m_labelFilterMode, m_comboFilterMode);
}

void OptionsPage::setupProtectionBox()
{
    m_cbBootFilter = ControlUtil::createCheckBox(conf()->bootFilter(), [&](bool checked) {
        conf()->setBootFilter(checked);
        ctrl()->setFlagsEdited();
    });

    m_cbFilterLocals = ControlUtil::createCheckBox(conf()->filterLocals(), [&](bool checked) {
        conf()->setFilterLocals(checked);
        ctrl()->setFlagsEdited();
    });

    m_cbNoServiceControl =
            ControlUtil::createCheckBox(ini()->noServiceControl(), [&](bool checked) {
                ini()->setNoServiceControl(checked);
                ctrl()->setIniEdited();
            });

    m_cbCheckPasswordOnUninstall =
            ControlUtil::createCheckBox(ini()->checkPasswordOnUninstall(), [&](bool checked) {
                ini()->setCheckPasswordOnUninstall(checked);
                ctrl()->setIniEdited();
            });

    m_cbCheckPasswordOnUninstall->setEnabled(settings()->hasMasterAdmin());

    // Password Row
    auto passwordLayout = setupPasswordLayout();
    setupPasswordLock();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbBootFilter);
    layout->addWidget(m_cbFilterLocals);
    layout->addWidget(m_cbNoServiceControl);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_cbCheckPasswordOnUninstall);
    layout->addLayout(passwordLayout);
    layout->addWidget(m_btPasswordLock, 0, Qt::AlignCenter);

    m_gbProtection = new QGroupBox();
    m_gbProtection->setLayout(layout);
}

QLayout *OptionsPage::setupPasswordLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(6);

    m_cbPassword = ControlUtil::createCheckBox(settings()->hasPassword(), [&](bool checked) {
        if (checked) {
            m_editPassword->setFocus();
        } else {
            m_editPassword->clear();
        }

        setPasswordEdited(true);
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
    m_btPasswordLock = ControlUtil::createToolButton(":/icons/lock_open.png", [&] {
        settings()->resetCheckedPassword();
        m_btPasswordLock->hide();
    });

    const auto refreshPasswordLock = [&] {
        m_btPasswordLock->setVisible(settings()->hasPassword()
                && settings()->passwordUnlockType() > FortSettings::UnlockWindow);
    };

    refreshPasswordLock();

    connect(settings(), &FortSettings::passwordCheckedChanged, this, refreshPasswordLock);
}

void OptionsPage::setupProgBox()
{
    setupLogBlocked();
    setupPurgeOnStart();

    // Layout
    auto layout = ControlUtil::createLayoutByWidgets({ m_cbLogBlocked, m_cbPurgeOnStart });

    m_gbProg = new QGroupBox();
    m_gbProg->setLayout(layout);
}

void OptionsPage::setupLogBlocked()
{
    m_cbLogBlocked = ControlUtil::createCheckBox(conf()->logBlocked(), [&](bool checked) {
        if (conf()->logBlocked() != checked) {
            conf()->setLogBlocked(checked);
            ctrl()->setFlagsEdited();
        }
    });

    m_cbLogBlocked->setFont(ControlUtil::fontDemiBold());
}

void OptionsPage::setupPurgeOnStart()
{
    m_cbPurgeOnStart = ControlUtil::createCheckBox(ini()->progPurgeOnStart(), [&](bool checked) {
        if (ini()->progPurgeOnStart() != checked) {
            ini()->setProgPurgeOnStart(checked);
            ctrl()->setIniEdited();
        }
    });
}

QLayout *OptionsPage::setupColumn2()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    // Global Group Box
    setupGlobalBox();
    layout->addWidget(m_gbGlobal);

    // Hot Keys Group Box
    setupHotKeysBox();
    layout->addWidget(m_gbHotKeys);

    // Home Group Box
    setupHomeBox();
    layout->addWidget(m_gbHome);

    // Tray Group Box
    setupTrayBox();
    layout->addWidget(m_gbTray);

    // Confirmations Group Box
    setupConfirmationsBox();
    layout->addWidget(m_gbConfirmations);

    // Logs Group Box
    setupLogsBox();
    layout->addWidget(m_gbLogs);

    layout->addStretch();

    return layout;
}

void OptionsPage::setupGlobalBox()
{
    m_cbExplorerMenu = ControlUtil::createCheckBox(ini()->explorerIntegrated(), [&](bool checked) {
        ini()->setExplorerIntegrated(checked);
        ctrl()->setIniEdited();
    });
    m_cbExplorerMenu->setEnabled(settings()->hasMasterAdmin());

    m_cbUseSystemLocale =
            ControlUtil::createCheckBox(iniUser()->useSystemLocale(), [&](bool checked) {
                iniUser()->setUseSystemLocale(checked);
                ctrl()->setIniUserEdited(true);
            });

    // Language Row
    auto langLayout = setupLangLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbExplorerMenu);
    layout->addWidget(m_cbUseSystemLocale);
    layout->addLayout(langLayout);

    m_gbGlobal = new QGroupBox();
    m_gbGlobal->setLayout(layout);
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
            ControlUtil::createComboBox(translationManager()->displayLabels(), [&](int index) {
                if (translationManager()->switchLanguage(index)) {
                    setLanguageEdited(true);
                    iniUser()->setLanguage(translationManager()->localeName());
                    ctrl()->setIniUserEdited();
                }
            });
    m_comboLanguage->setFixedWidth(200);

    const auto refreshComboLanguage = [&] {
        m_comboLanguage->setCurrentIndex(translationManager()->language());
    };

    refreshComboLanguage();

    connect(translationManager(), &TranslationManager::languageChanged, this, refreshComboLanguage);
}

void OptionsPage::setupHotKeysBox()
{
    m_cbHotKeysEnabled = ControlUtil::createCheckBox(iniUser()->hotKeyEnabled(), [&](bool checked) {
        iniUser()->setHotKeyEnabled(checked);
        ctrl()->setIniUserEdited(true);
    });

    m_cbHotKeysGlobal = ControlUtil::createCheckBox(iniUser()->hotKeyGlobal(), [&](bool checked) {
        iniUser()->setHotKeyGlobal(checked);
        ctrl()->setIniUserEdited(true);
    });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbHotKeysEnabled);
    layout->addWidget(m_cbHotKeysGlobal);

    m_gbHotKeys = new QGroupBox();
    m_gbHotKeys->setLayout(layout);
}

void OptionsPage::setupHomeBox()
{
    m_cbHomeAutoShowMenu =
            ControlUtil::createCheckBox(iniUser()->homeAutoShowMenu(), [&](bool checked) {
                iniUser()->setHomeAutoShowMenu(checked);
                ctrl()->setIniUserEdited();
            });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbHomeAutoShowMenu);

    m_gbHome = new QGroupBox();
    m_gbHome->setLayout(layout);
}

void OptionsPage::setupTrayBox()
{
    m_cbTrayShowIcon = ControlUtil::createCheckBox(iniUser()->trayShowIcon(), [&](bool checked) {
        iniUser()->setTrayShowIcon(checked);
        ctrl()->setIniUserEdited(true);
    });

    m_cbTrayAnimateAlert =
            ControlUtil::createCheckBox(iniUser()->trayAnimateAlert(), [&](bool checked) {
                iniUser()->setTrayAnimateAlert(checked);
                ctrl()->setIniUserEdited();
            });

    // Tray Event & Action Rows
    auto eventLayout = setupTrayEventLayout();
    auto actionLayout = setupTrayActionLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbTrayShowIcon);
    layout->addWidget(m_cbTrayAnimateAlert);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(eventLayout);
    layout->addLayout(actionLayout);

    m_gbTray = new QGroupBox();
    m_gbTray->setLayout(layout);
}

void OptionsPage::refreshComboTrayAction()
{
    const TrayIcon::ClickType clickType =
            static_cast<TrayIcon::ClickType>(m_comboTrayEvent->currentIndex());

    const TrayIcon::ActionType actionType = TrayIcon::clickEventActionType(iniUser(), clickType);

    m_comboTrayAction->setCurrentIndex(actionType);
}

QLayout *OptionsPage::setupTrayEventLayout()
{
    m_labelTrayEvent = ControlUtil::createLabel();

    m_comboTrayEvent = ControlUtil::createComboBox(
            QStringList(), [&](int /*index*/) { refreshComboTrayAction(); });
    m_comboTrayEvent->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelTrayEvent, m_comboTrayEvent);
}

QLayout *OptionsPage::setupTrayActionLayout()
{
    m_labelTrayAction = ControlUtil::createLabel();

    m_comboTrayAction = ControlUtil::createComboBox(QStringList(), [&](int index) {
        const TrayIcon::ClickType clickType =
                static_cast<TrayIcon::ClickType>(m_comboTrayEvent->currentIndex());
        const TrayIcon::ActionType actionType = static_cast<TrayIcon::ActionType>(index);

        TrayIcon::setClickEventActionType(iniUser(), clickType, actionType);
        ctrl()->setIniUserEdited(true);
    });
    m_comboTrayAction->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelTrayAction, m_comboTrayAction);
}

void OptionsPage::setupConfirmationsBox()
{
    m_cbConfirmTrayFlags =
            ControlUtil::createCheckBox(iniUser()->confirmTrayFlags(), [&](bool checked) {
                iniUser()->setConfirmTrayFlags(checked);
                ctrl()->setIniUserEdited();
            });
    m_cbConfirmQuit = ControlUtil::createCheckBox(iniUser()->confirmQuit(), [&](bool checked) {
        iniUser()->setConfirmQuit(checked);
        ctrl()->setIniUserEdited();
    });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbConfirmTrayFlags);
    layout->addWidget(m_cbConfirmQuit);

    m_gbConfirmations = new QGroupBox();
    m_gbConfirmations->setLayout(layout);
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

    m_gbLogs = new QGroupBox();
    m_gbLogs->setLayout(layout);
}
