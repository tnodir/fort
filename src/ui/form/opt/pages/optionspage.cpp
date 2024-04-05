#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
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
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/osutil.h>
#include <util/startuputil.h>

namespace {

constexpr int trayMaxGroups = 16;

struct Startup
{
    quint8 initialized : 1 = false;
    quint8 isServiceChanged : 1 = false;
    quint8 wasService : 1 = false;
    quint8 isService : 1 = false;
} g_startup;

}

OptionsPage::OptionsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupStartup();
    setupUi();
}

void OptionsPage::onResetToDefault()
{
    m_cbFilterEnabled->setChecked(true);
    m_cbBlockTraffic->setChecked(false);
    m_cbBlockInetTraffic->setChecked(false);

    m_cbLogBlocked->setChecked(true);
    m_cbAppNotifyMessage->setChecked(true);
    m_cbAppAlertAutoShow->setChecked(true);
    m_cbAppAlertAlwaysOnTop->setChecked(false);
    m_cbPurgeOnMounted->setChecked(false);

    m_cbUseSystemLocale->setChecked(true);
    m_cbHotKeysEnabled->setChecked(false);
    m_cbHotKeysGlobal->setChecked(true);

    // Reset Shortcuts
    for (int i = 0; i < HotKey::listCount; ++i) {
        const auto &key = HotKey::list[i];
        iniUser()->setHotKeyValue(key, HotKey::defaultValue(key));
    }
    refreshEditShortcut();

    m_cbHomeAutoShowMenu->setChecked(false);
    m_cbSplashVisible->setChecked(true);
    m_cbTrayShowIcon->setChecked(true);
    m_cbTrayAnimateAlert->setChecked(true);

    m_spinTrayMaxGroups->setValue(trayMaxGroups);

    // Reset Tray Actions
    for (int i = 0; i < TrayIcon::ClickTypeCount; ++i) {
        TrayIcon::resetClickEventActionType(iniUser(), TrayIcon::ClickType(i));
    }
    refreshComboTrayAction();

    m_cbConfirmTrayFlags->setChecked(false);
    m_cbConfirmQuit->setChecked(true);

    m_cbLogDebug->setChecked(false);
    m_cbLogConsole->setChecked(false);
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

    // Explorer
    if (explorerEdited()) {
        StartupUtil::setExplorerIntegrated(m_cbExplorerMenu->isChecked());
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

    // Explorer
    if (explorerEdited()) {
        setExplorerEdited(false);
        m_cbExplorerMenu->setChecked(StartupUtil::isExplorerIntegrated());
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

        connect(fortManager(), &QObject::destroyed, [=] {
            if (g_startup.wasService == g_startup.isService)
                return;

            StartupUtil::setServiceInstalled(false);

            if (g_startup.isService) {
                StartupUtil::setServiceInstalled(true);
            }
        });
    }

    fortManager()->processRestartRequired(tr("Windows Service installation changed"));
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

    m_cbService->setText(tr("Windows Service"));
    m_cbService->setToolTip(tr("Run Fort Firewall as a Service in background"));

    m_cbFilterEnabled->setText(tr("Filter Enabled"));
    m_cbBlockTraffic->setText(tr("Block All Traffic"));
    m_cbBlockInetTraffic->setText(tr("Block Internet Traffic"));

    m_labelFilterMode->setText(tr("Filter Mode:"));
    retranslateComboFilterMode();

    m_cbBootFilter->setText(tr("Block traffic when Fort Firewall is not running"));
    m_cbFilterLocals->setText(tr("Filter Local Addresses"));
    m_cbFilterLocals->setToolTip(
            tr("Filter Local Loopback (127.0.0.0/8) and Broadcast (255.255.255.255) Addresses"));
    m_cbNoServiceControl->setText(tr("Disable Service controls"));
    m_cbCheckPasswordOnUninstall->setText(tr("Check password on Uninstall"));

    m_cbPassword->setText(tr("Password:"));
    retranslateEditPassword();

    m_btPasswordLock->setText(tr("Lock the password (unlocked till \"%1\")")
                                      .arg(settings()->passwordUnlockedTillText()));

    m_cbLogBlocked->setText(tr("Collect New Programs"));
    m_cbAppNotifyMessage->setText(tr("Use System Notifications for New Programs"));
    m_cbAppAlertAutoShow->setText(tr("Auto-Show Alert Window for New Programs"));
    m_cbAppAlertAlwaysOnTop->setText(tr("Alert Window is Always on top"));
    m_cbPurgeOnMounted->setText(tr("Purge Obsolete only on mounted drives"));

    m_cbExplorerMenu->setText(tr("Windows Explorer integration"));
    m_cbUseSystemLocale->setText(tr("Use System Regional Settings"));
    m_labelLanguage->setText(tr("Language:"));

    m_cbHotKeysEnabled->setText(tr("Enabled"));
    m_cbHotKeysGlobal->setText(tr("Global"));
    m_labelHotKey->setText(tr("Hot Key:"));
    m_labelShortcut->setText(tr("Shortcut:"));
    retranslateComboHotKey();
    refreshEditShortcut();

    m_cbHomeAutoShowMenu->setText(tr("Auto-Show Menu"));
    m_cbSplashVisible->setText(tr("Show Splash screen on startup"));

    m_cbTrayShowIcon->setText(tr("Show Icon"));
    m_cbTrayAnimateAlert->setText(tr("Animate Alert Icon"));
    m_labelTrayMaxGroups->setText(tr("Maximum count of Groups in menu:"));
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

    ControlUtil::setComboBoxTexts(m_comboAutoRun, list, currentIndex);

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

void OptionsPage::retranslateComboHotKey()
{
    // Sync with TrayIcon::retranslateUi() & HotKey::list[]
    QStringList list = { TrayIcon::tr("My Fort"), TrayIcon::tr("Programs"), TrayIcon::tr("Options"),
        TrayIcon::tr("Rules"), TrayIcon::tr("Zones"), TrayIcon::tr("Statistics"),
        TrayIcon::tr("Traffic Graph"), TrayIcon::tr("Filter Enabled"),
        TrayIcon::tr("Block All Traffic"), TrayIcon::tr("Block Internet Traffic") };

    const auto filterMode = tr("Filter Mode:");
    for (const auto &filterModeName : FirewallConf::filterModeNames()) {
        list.append(filterMode + ' ' + filterModeName);
    }

    list.append({ TrayIcon::tr("App Group Modifier"), TrayIcon::tr("Quit") });

    const int currentIndex = qMax(m_comboHotKey->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboHotKey, list, currentIndex);
}

void OptionsPage::retranslateComboTrayEvent()
{
    // Sync with TrayIcon::ClickType
    const QStringList list = { tr("Single Click"), tr("Ctrl + Single Click"),
        tr("Alt + Single Click"), tr("Double Click"), tr("Middle Click"), tr("Right Click") };

    const int currentIndex = qMax(m_comboTrayEvent->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboTrayEvent, list, currentIndex);
}

void OptionsPage::retranslateComboTrayAction()
{
    const TrayIcon::ActionType type = TrayIcon::ActionNone; // to find the enum usages
    Q_UNUSED(type);

    // Sync with TrayIcon::ActionType
    const QStringList list = { tr("Show My Fort"), tr("Show Programs"),
        tr("Show Programs Or Alert Window"), tr("Show Options"), tr("Show Statistics"),
        tr("Show/Hide Traffic Graph"), tr("Switch Filter Enabled"), tr("Switch Block All Traffic"),
        tr("Switch Block Internet Traffic"), tr("Show Filter Mode Menu"), tr("Show Tray Menu"),
        tr("Ignore") };

    ControlUtil::setComboBoxTexts(m_comboTrayAction, list, /*currentIndex=*/-1);
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
    m_cbBlockTraffic = ControlUtil::createCheckBox(conf()->blockTraffic(), [&](bool checked) {
        conf()->setBlockTraffic(checked);
        ctrl()->setFlagsEdited();
    });
    m_cbBlockInetTraffic =
            ControlUtil::createCheckBox(conf()->blockInetTraffic(), [&](bool checked) {
                conf()->setBlockInetTraffic(checked);
                ctrl()->setFlagsEdited();
            });

    auto filterModeLayout = setupFilterModeLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addWidget(m_cbBlockTraffic);
    layout->addWidget(m_cbBlockInetTraffic);
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

    m_cbAppNotifyMessage =
            ControlUtil::createCheckBox(iniUser()->progNotifyMessage(), [&](bool checked) {
                iniUser()->setProgNotifyMessage(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbAppAlertAutoShow =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAutoShow(), [&](bool checked) {
                iniUser()->setProgAlertWindowAutoShow(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbAppAlertAlwaysOnTop =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAlwaysOnTop(), [&](bool checked) {
                iniUser()->setProgAlertWindowAlwaysOnTop(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbPurgeOnMounted =
            ControlUtil::createCheckBox(ini()->progPurgeOnMounted(), [&](bool checked) {
                if (ini()->progPurgeOnMounted() != checked) {
                    ini()->setProgPurgeOnMounted(checked);
                    ctrl()->setIniEdited();
                }
            });

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbLogBlocked, m_cbAppNotifyMessage, m_cbAppAlertAutoShow, m_cbAppAlertAlwaysOnTop,
                    ControlUtil::createSeparator(), m_cbPurgeOnMounted });

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

    m_cbLogBlocked->setFont(GuiUtil::fontBold());
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
    m_cbExplorerMenu =
            ControlUtil::createCheckBox(StartupUtil::isExplorerIntegrated(), [&](bool /*checked*/) {
                setExplorerEdited(true);
                ctrl()->setIniUserEdited();
            });

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
    m_comboLanguage = ControlUtil::createComboBox({}, [&](int index) {
        if (translationManager()->switchLanguage(index)) {
            setLanguageEdited(true);
            iniUser()->setLanguage(translationManager()->languageName());
            ctrl()->setIniUserEdited();
        }
    });
    m_comboLanguage->setFixedWidth(200);

    const auto refreshComboLanguage = [&] {
        ControlUtil::setComboBoxTexts(m_comboLanguage, translationManager()->displayLabels(),
                translationManager()->language());
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

    // Hot Keys Combo & Edit Rows
    auto comboLayout = setupComboHotKeyLayout();
    auto editLayout = setupEditShortcutLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbHotKeysEnabled);
    layout->addWidget(m_cbHotKeysGlobal);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(comboLayout);
    layout->addLayout(editLayout);

    m_gbHotKeys = new QGroupBox();
    m_gbHotKeys->setLayout(layout);
}

void OptionsPage::refreshEditShortcut()
{
    const auto &key = HotKey::list[m_comboHotKey->currentIndex()];

    m_editShortcut->setKeySequence(iniUser()->hotKeyValue(key));
}

QLayout *OptionsPage::setupComboHotKeyLayout()
{
    m_labelHotKey = ControlUtil::createLabel();

    m_comboHotKey = ControlUtil::createComboBox(
            QStringList(), [&](int /*index*/) { refreshEditShortcut(); });
    m_comboHotKey->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelHotKey, m_comboHotKey);
}

QLayout *OptionsPage::setupEditShortcutLayout()
{
    m_labelShortcut = ControlUtil::createLabel();

    m_editShortcut = new QKeySequenceEdit();
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    m_editShortcut->setClearButtonEnabled(true);
#endif
    m_editShortcut->setFixedWidth(200);

    const auto onEditShortcut = [&](const QKeySequence &shortcut) {
        const auto &key = HotKey::list[m_comboHotKey->currentIndex()];
        const auto value = shortcut.toString();

        if (value == iniUser()->hotKeyValue(key))
            return;

        iniUser()->setHotKeyValue(key, value);
        ctrl()->setIniUserEdited();
    };

    connect(m_editShortcut, &QKeySequenceEdit::editingFinished,
            [=, this] { onEditShortcut(m_editShortcut->keySequence()); });

    connect(m_editShortcut, &QKeySequenceEdit::keySequenceChanged,
            [=, this](const QKeySequence &shortcut) {
                if (shortcut.isEmpty()) {
                    onEditShortcut({});
                }
            });

    return ControlUtil::createRowLayout(m_labelShortcut, m_editShortcut);
}

void OptionsPage::setupHomeBox()
{
    m_cbHomeAutoShowMenu =
            ControlUtil::createCheckBox(iniUser()->homeAutoShowMenu(), [&](bool checked) {
                iniUser()->setHomeAutoShowMenu(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbSplashVisible =
            ControlUtil::createCheckBox(iniUser()->splashWindowVisible(), [&](bool checked) {
                iniUser()->setSplashWindowVisible(checked);
                ctrl()->setIniUserEdited();
            });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbHomeAutoShowMenu);
    layout->addWidget(m_cbSplashVisible);

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

    // Tray Max. Groups Row
    auto maxGroupsLayout = setupTrayMaxGroupsLayout();

    // Tray Event & Action Rows
    auto eventLayout = setupTrayEventLayout();
    auto actionLayout = setupTrayActionLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbTrayShowIcon);
    layout->addWidget(m_cbTrayAnimateAlert);
    layout->addLayout(maxGroupsLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(eventLayout);
    layout->addLayout(actionLayout);

    m_gbTray = new QGroupBox();
    m_gbTray->setLayout(layout);
}

QLayout *OptionsPage::setupTrayMaxGroupsLayout()
{
    m_labelTrayMaxGroups = ControlUtil::createLabel();

    m_spinTrayMaxGroups = ControlUtil::createSpinBox();
    m_spinTrayMaxGroups->setFixedWidth(50);

    m_spinTrayMaxGroups->setRange(0, trayMaxGroups);
    m_spinTrayMaxGroups->setValue(iniUser()->trayMaxGroups(trayMaxGroups));

    connect(m_spinTrayMaxGroups, QOverload<int>::of(&QSpinBox::valueChanged), [&](int v) {
        iniUser()->setTrayMaxGroups(v);
        ctrl()->setIniUserEdited();
    });

    return ControlUtil::createRowLayout(m_labelTrayMaxGroups, m_spinTrayMaxGroups);
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
