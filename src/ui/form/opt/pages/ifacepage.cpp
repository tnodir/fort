#include "ifacepage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
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

namespace {

constexpr int trayMaxGroups = 16;

void setAlertModeIcon(QCheckBox *cb, FirewallConf::FilterMode mode)
{
    cb->setIcon(IconCache::icon(FirewallConf::filterModeIconPaths().value(mode)));
}

void setAlertModeText(QCheckBox *cb, FirewallConf::FilterMode mode)
{
    cb->setText(FirewallConf::filterModeNames().value(mode));
}

}

IfacePage::IfacePage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void IfacePage::onResetToDefault()
{
    m_comboLanguage->setCurrentIndex(0);
    m_comboTheme->setCurrentIndex(0);
    m_comboStyle->setCurrentText(IniUser::styleDefault());
    m_cbUseSystemLocale->setChecked(true);
    m_cbExcludeCapture->setChecked(false);

    m_cbHotKeysEnabled->setChecked(false);
    m_cbHotKeysGlobal->setChecked(true);

    // Reset Shortcuts
    for (int i = 0; i < HotKey::listCount; ++i) {
        const auto &key = HotKey::list[i];
        iniUser()->setHotKeyValue(key, HotKey::defaultValue(key));
    }
    refreshEditShortcut();

    m_cbHomeAutoShowWindow->setChecked(true);
    m_cbHomeAutoShowMenu->setChecked(false);
    m_cbSplashVisible->setChecked(true);
    m_cbUpdateWindowIcons->setChecked(false);

    m_cbAppNotifyMessage->setChecked(true);
    m_cbAppAlertAutoShow->setChecked(true);
    m_cbAppAlertAutoLearn->setChecked(false);
    m_cbAppAlertBlockAll->setChecked(true);
    m_cbAppAlertAllowAll->setChecked(true);
    m_cbAppAlertAlwaysOnTop->setChecked(true);
    m_cbAppAlertAutoActive->setChecked(false);
    m_cbAppAlertAutoClear->setChecked(false);
    m_cbAppAlertSound->setChecked(true);
    m_cbSnoozeAlerts->setChecked(false);

    m_cbTrayShowIcon->setChecked(true);
    m_cbTrayShowAlert->setChecked(true);
    m_cbTrayAnimateAlert->setChecked(true);

    m_spinTrayMaxGroups->setValue(trayMaxGroups);

    // Reset Tray Actions
    for (int i = 0; i < TrayIcon::ClickTypeCount; ++i) {
        TrayIcon::resetClickEventActionType(iniUser(), TrayIcon::ClickType(i));
    }
    refreshComboTrayAction();

    m_cbConfirmTrayFlags->setChecked(false);
    m_cbConfirmQuit->setChecked(true);
}

void IfacePage::onEditResetted()
{
    // Language
    if (languageEdited()) {
        setLanguageEdited(false);
        translationManager()->switchLanguageByName(confManager()->iniUser().language());
    }

    // Theme
    if (themeEdited()) {
        setThemeEdited(false);
        WindowManager::updateTheme(confManager()->iniUser());
    }

    // Style
    if (styleEdited()) {
        setStyleEdited(false);
        WindowManager::updateStyle(confManager()->iniUser());
    }
}

void IfacePage::onRetranslateUi()
{
    m_gbGlobal->setTitle(tr("Global"));
    m_gbHotKeys->setTitle(tr("Hot Keys"));
    m_gbHome->setTitle(tr("My Fort"));
    m_gbProg->setTitle(tr("Programs"));
    m_gbTray->setTitle(tr("Tray"));
    m_gbConfirmations->setTitle(tr("Action Confirmations"));

    m_labelLanguage->setText(tr("Language:"));
    retranslateComboLanguage();
    m_labelTheme->setText(tr("Theme:"));
    retranslateComboTheme();
    m_labelStyle->setText(tr("Style:"));
    retranslateComboStyle();
    m_cbUseSystemLocale->setText(tr("Use System Regional Settings"));
    m_cbExcludeCapture->setText(tr("Exclude from screen capture"));

    m_cbHotKeysEnabled->setText(tr("Enabled"));
    m_cbHotKeysGlobal->setText(tr("Global"));
    m_labelHotKey->setText(tr("Hot Key:"));
    m_labelShortcut->setText(tr("Shortcut:"));
    retranslateComboHotKey();
    refreshEditShortcut();

    m_cbHomeAutoShowWindow->setText(tr("Auto-Show Window"));
    m_cbHomeAutoShowWindow->setToolTip(tr("Auto-Show Window, when Tray Icon is hidden"));
    m_cbHomeAutoShowMenu->setText(tr("Auto-Show Menu"));
    m_cbSplashVisible->setText(tr("Show Splash screen on startup"));
    m_cbUpdateWindowIcons->setText(tr("Show window icons as in tray icon"));

    m_cbAppNotifyMessage->setText(tr("Use System Notifications for New Programs"));
    m_cbAppAlertAutoShow->setText(tr("Auto-Show Alert Window for New Programs"));
    setAlertModeText(m_cbAppAlertAutoLearn, FirewallConf::ModeAutoLearn);
    setAlertModeText(m_cbAppAlertBlockAll, FirewallConf::ModeBlockAll);
    setAlertModeText(m_cbAppAlertAllowAll, FirewallConf::ModeAllowAll);
    m_cbAppAlertAlwaysOnTop->setText(tr("Alert Window is Always on top"));
    m_cbAppAlertAutoActive->setText(tr("Alert Window is auto-active"));
    m_cbAppAlertAutoClear->setText(tr("Auto-Clear alerts on window close"));
    m_cbAppAlertSound->setText(tr("Sound Alert"));
    m_cbSnoozeAlerts->setText(tr("Snooze Alerts"));

    m_cbTrayShowIcon->setText(tr("Show Icon"));
    m_cbTrayShowAlert->setText(tr("Show Alert Icon"));
    m_cbTrayAnimateAlert->setText(tr("Animate Alert Icon"));
    m_labelTrayMaxGroups->setText(tr("Maximum count of Groups in menu:"));
    m_labelTrayEvent->setText(tr("Event:"));
    m_labelTrayAction->setText(tr("Action:"));
    retranslateComboTrayEvent();
    retranslateComboTrayAction();
    refreshComboTrayAction();

    m_cbConfirmTrayFlags->setText(tr("Operations from Tray Menu"));
    m_cbConfirmQuit->setText(tr("Quit"));
}

void IfacePage::retranslateComboLanguage()
{
    ControlUtil::setComboBoxTexts(m_comboLanguage, translationManager()->displayLabels(),
            translationManager()->language());
}

void IfacePage::retranslateComboTheme()
{
    // Sync with Qt::ColorScheme
    const QStringList list = { tr("System"), tr("Light"), tr("Dark") };

    ControlUtil::setComboBoxTexts(m_comboTheme, list);

    updateTheme();
}

void IfacePage::retranslateComboStyle()
{
    const QStringList list = { IniUser::styleDefault(), "Windows11", "WindowsVista", "Windows" };

    ControlUtil::setComboBoxTexts(m_comboStyle, list);

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    ControlUtil::setComboBoxItemsEnabled(m_comboStyle, /*enabled=*/false, 1, 2);
#endif

    updateStyle();
}

void IfacePage::retranslateComboHotKey()
{
    // Sync with TrayIcon::retranslateUi() & HotKey::list[]
    QStringList list = { TrayIcon::tr("My Fort"), TrayIcon::tr("Programs"), TrayIcon::tr("Options"),
        TrayIcon::tr("Rules"), TrayIcon::tr("Zones"), TrayIcon::tr("Groups"),
        TrayIcon::tr("Services"), TrayIcon::tr("Statistics"), TrayIcon::tr("Traffic Graph"),
        TrayIcon::tr("Filter Enabled"), TrayIcon::tr("Snooze Alerts") };

    const auto blockTraffic = tr("Block Traffic:");
    for (const auto &name : FirewallConf::blockTrafficNames()) {
        list.append(blockTraffic + ' ' + name);
    }

    const auto filterMode = tr("Filter Mode:");
    for (const auto &name : FirewallConf::filterModeNames()) {
        list.append(filterMode + ' ' + name);
    }

    list.append({ TrayIcon::tr("App Group Modifier"), TrayIcon::tr("Quit") });

    const int currentIndex = qMax(m_comboHotKey->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboHotKey, list, currentIndex);
}

void IfacePage::retranslateComboTrayEvent()
{
    // Sync with TrayIcon::ClickType
    const QStringList list = { tr("Single Click"), tr("Ctrl + Single Click"),
        tr("Alt + Single Click"), tr("Double Click"), tr("Middle Click"), tr("Right Click") };

    const int currentIndex = qMax(m_comboTrayEvent->currentIndex(), 0);

    ControlUtil::setComboBoxTexts(m_comboTrayEvent, list, currentIndex);
}

void IfacePage::retranslateComboTrayAction()
{
    const TrayIcon::ActionType type = TrayIcon::ActionNone; // to find the enum usages
    Q_UNUSED(type);

    // Sync with TrayIcon::ActionType
    const QStringList list = { tr("Show My Fort"), tr("Show Programs"),
        tr("Show Programs Or Alert Window"), tr("Show Options"), tr("Show Statistics"),
        tr("Show/Hide Traffic Graph"), tr("Switch Filter Enabled"), tr("Switch Snooze Alerts"),
        tr("Show Block Traffic Menu"), tr("Show Filter Mode Menu"), tr("Show Tray Menu"),
        tr("Ignore") };

    ControlUtil::setComboBoxTexts(m_comboTrayAction, list, /*currentIndex=*/-1);
}

void IfacePage::setupUi()
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

QLayout *IfacePage::setupColumn1()
{
    // Global Group Box
    setupGlobalBox();

    // Hot Keys Group Box
    setupHotKeysBox();

    // Home Group Box
    setupHomeBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbGlobal);
    layout->addWidget(m_gbHotKeys);
    layout->addWidget(m_gbHome);
    layout->addStretch();

    return layout;
}

QLayout *IfacePage::setupColumn2()
{
    // Programs Group Box
    setupProgBox();

    // Tray Group Box
    setupTrayBox();

    // Confirmations Group Box
    setupConfirmationsBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbProg);
    layout->addWidget(m_gbTray);
    layout->addWidget(m_gbConfirmations);
    layout->addStretch();

    return layout;
}

void IfacePage::setupGlobalBox()
{
    // Language Row
    auto langLayout = setupLangLayout();

    // Theme Row
    auto themeLayout = setupThemeLayout();

    // Style Row
    auto styleLayout = setupStyleLayout();

    m_cbUseSystemLocale =
            ControlUtil::createCheckBox(iniUser()->useSystemLocale(), [&](bool checked) {
                iniUser()->setUseSystemLocale(checked);
                ctrl()->setIniUserEdited(true);
            });

    m_cbExcludeCapture =
            ControlUtil::createCheckBox(iniUser()->excludeFromCapture(), [&](bool checked) {
                iniUser()->setExcludeFromCapture(checked);
                ctrl()->setIniUserEdited();
            });

    auto layout = new QVBoxLayout();
    layout->addLayout(langLayout);
    layout->addLayout(themeLayout);
    layout->addLayout(styleLayout);
    layout->addWidget(m_cbUseSystemLocale);
    layout->addWidget(m_cbExcludeCapture);

    m_gbGlobal = new QGroupBox();
    m_gbGlobal->setLayout(layout);
}

QLayout *IfacePage::setupLangLayout()
{
    // Language Label
    m_labelLanguage = ControlUtil::createLabel();

    // Language Combobox
    m_comboLanguage = ControlUtil::createComboBox({}, [&](int index) {
        if (translationManager()->switchLanguage(index)) {
            setLanguageEdited(true);
            iniUser()->setLanguage(translationManager()->languageName());
            ctrl()->setIniUserEdited();
        }
    });
    m_comboLanguage->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelLanguage, m_comboLanguage);
}

QLayout *IfacePage::setupThemeLayout()
{
    m_labelTheme = ControlUtil::createLabel();

    m_comboTheme = ControlUtil::createComboBox({}, [&](int index) {
        const auto theme = IniUser::colorSchemeName(index);

        if (iniUser()->theme() != theme) {
            setThemeEdited(true);
            iniUser()->setTheme(theme);
            ctrl()->setIniUserEdited();

            WindowManager::updateTheme(*iniUser());
        }
    });
    m_comboTheme->setFixedWidth(200);

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    m_comboTheme->setEnabled(false);
#endif

    return ControlUtil::createRowLayout(m_labelTheme, m_comboTheme);
}

QLayout *IfacePage::setupStyleLayout()
{
    m_labelStyle = ControlUtil::createLabel();

    m_comboStyle = ControlUtil::createComboBox({}, [&](int index) {
        const auto style = m_comboStyle->itemText(index);

        if (iniUser()->style() != style) {
            setStyleEdited(true);
            iniUser()->setStyle(style);
            ctrl()->setIniUserEdited();

            WindowManager::updateStyle(*iniUser());
        }
    });
    m_comboStyle->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelStyle, m_comboStyle);
}

void IfacePage::setupHotKeysBox()
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

void IfacePage::refreshEditShortcut()
{
    const auto &key = HotKey::list[m_comboHotKey->currentIndex()];

    m_editShortcut->setKeySequence(iniUser()->hotKeyValue(key));
}

QLayout *IfacePage::setupComboHotKeyLayout()
{
    m_labelHotKey = ControlUtil::createLabel();

    m_comboHotKey = ControlUtil::createComboBox(
            QStringList(), [&](int /*index*/) { refreshEditShortcut(); });
    m_comboHotKey->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelHotKey, m_comboHotKey);
}

QLayout *IfacePage::setupEditShortcutLayout()
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

void IfacePage::setupHomeBox()
{
    m_cbHomeAutoShowWindow =
            ControlUtil::createCheckBox(iniUser()->homeWindowAutoShowWindow(), [&](bool checked) {
                iniUser()->setHomeWindowAutoShowWindow(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbHomeAutoShowMenu =
            ControlUtil::createCheckBox(iniUser()->homeWindowAutoShowMenu(), [&](bool checked) {
                iniUser()->setHomeWindowAutoShowMenu(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbSplashVisible =
            ControlUtil::createCheckBox(iniUser()->splashWindowVisible(), [&](bool checked) {
                iniUser()->setSplashWindowVisible(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbUpdateWindowIcons =
            ControlUtil::createCheckBox(iniUser()->updateWindowIcons(), [&](bool checked) {
                iniUser()->setUpdateWindowIcons(checked);
                ctrl()->setIniUserEdited();
            });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbHomeAutoShowWindow);
    layout->addWidget(m_cbHomeAutoShowMenu);
    layout->addWidget(m_cbSplashVisible);
    layout->addWidget(m_cbUpdateWindowIcons);

    m_gbHome = new QGroupBox();
    m_gbHome->setLayout(layout);
}

void IfacePage::setupProgBox()
{
    m_cbAppNotifyMessage =
            ControlUtil::createCheckBox(iniUser()->progNotifyMessage(), [&](bool checked) {
                iniUser()->setProgNotifyMessage(checked);
                ctrl()->setIniUserEdited();
            });

    // Alert layout
    auto alertLayout = setupAlertLayout();

    m_cbAppAlertAlwaysOnTop =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAlwaysOnTop(), [&](bool checked) {
                iniUser()->setProgAlertWindowAlwaysOnTop(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbAppAlertAutoActive =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAutoActive(), [&](bool checked) {
                iniUser()->setProgAlertWindowAutoActive(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbAppAlertAutoClear =
            ControlUtil::createCheckBox(iniUser()->progWindowAutoClearAlerts(), [&](bool checked) {
                iniUser()->setProgWindowAutoClearAlerts(checked);
                ctrl()->setIniUserEdited();
            });

    m_cbAppAlertSound = ControlUtil::createCheckBox(iniUser()->progAlertSound(), [&](bool checked) {
        iniUser()->setProgAlertSound(checked);
        ctrl()->setIniUserEdited();
    });

    m_cbSnoozeAlerts =
            ControlUtil::createCheckBox(iniUser()->progSnoozeAlerts(), [&](bool checked) {
                iniUser()->setProgSnoozeAlerts(checked);
                ctrl()->setIniUserEdited();
            });

    // Layout
    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbAppNotifyMessage);
    layout->addLayout(alertLayout);
    layout->addWidget(m_cbAppAlertAlwaysOnTop);
    layout->addWidget(m_cbAppAlertAutoActive);
    layout->addWidget(m_cbAppAlertAutoClear);
    layout->addWidget(m_cbAppAlertSound);
    layout->addWidget(m_cbSnoozeAlerts);

    m_gbProg = new QGroupBox();
    m_gbProg->setLayout(layout);
}

QLayout *IfacePage::setupAlertLayout()
{
    m_cbAppAlertAutoShow =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAutoShow(), [&](bool checked) {
                iniUser()->setProgAlertWindowAutoShow(checked);
                ctrl()->setIniUserEdited();
            });

    // Alert Modes Box
    setupAlertModes();

    auto layout = ControlUtil::createRowLayout(m_cbAppAlertAutoShow, m_btAlertModes);

    return layout;
}

void IfacePage::setupAlertModes()
{
    m_cbAppAlertAutoLearn =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAutoLearn(), [&](bool checked) {
                iniUser()->setProgAlertWindowAutoLearn(checked);
                ctrl()->setIniUserEdited();
            });
    setAlertModeIcon(m_cbAppAlertAutoLearn, FirewallConf::ModeAutoLearn);

    m_cbAppAlertBlockAll =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowBlockAll(), [&](bool checked) {
                iniUser()->setProgAlertWindowBlockAll(checked);
                ctrl()->setIniUserEdited();
            });
    setAlertModeIcon(m_cbAppAlertBlockAll, FirewallConf::ModeBlockAll);

    m_cbAppAlertAllowAll =
            ControlUtil::createCheckBox(iniUser()->progAlertWindowAllowAll(), [&](bool checked) {
                iniUser()->setProgAlertWindowAllowAll(checked);
                ctrl()->setIniUserEdited();
            });
    setAlertModeIcon(m_cbAppAlertAllowAll, FirewallConf::ModeAllowAll);

    // Setup Alert Modes Button
    setupAlertModesButton();
}

void IfacePage::setupAlertModesButton()
{
    // Menu
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbAppAlertAutoLearn, m_cbAppAlertBlockAll, m_cbAppAlertAllowAll });

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btAlertModes = ControlUtil::createButton(":/icons/widgets.png");
    m_btAlertModes->setMenu(menu);

    // Refresh Alert Modes Button
    const auto refreshAlertModes = [&] {
        m_btAlertModes->setEnabled(m_cbAppAlertAutoShow->isChecked());
    };

    refreshAlertModes();

    connect(m_cbAppAlertAutoShow, &QCheckBox::toggled, this, refreshAlertModes);
}

void IfacePage::setupTrayBox()
{
    m_cbTrayShowIcon = ControlUtil::createCheckBox(iniUser()->trayShowIcon(), [&](bool checked) {
        iniUser()->setTrayShowIcon(checked);
        ctrl()->setIniUserEdited(true);
    });

    m_cbTrayShowAlert = ControlUtil::createCheckBox(iniUser()->trayShowAlert(), [&](bool checked) {
        iniUser()->setTrayShowAlert(checked);
        ctrl()->setIniUserEdited();
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
    layout->addWidget(m_cbTrayShowAlert);
    layout->addWidget(m_cbTrayAnimateAlert);
    layout->addLayout(maxGroupsLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(eventLayout);
    layout->addLayout(actionLayout);

    m_gbTray = new QGroupBox();
    m_gbTray->setLayout(layout);
}

QLayout *IfacePage::setupTrayMaxGroupsLayout()
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

void IfacePage::refreshComboTrayAction()
{
    const TrayIcon::ClickType clickType =
            static_cast<TrayIcon::ClickType>(m_comboTrayEvent->currentIndex());

    const TrayIcon::ActionType actionType = TrayIcon::clickEventActionType(iniUser(), clickType);

    m_comboTrayAction->setCurrentIndex(actionType);
}

QLayout *IfacePage::setupTrayEventLayout()
{
    m_labelTrayEvent = ControlUtil::createLabel();

    m_comboTrayEvent = ControlUtil::createComboBox(
            QStringList(), [&](int /*index*/) { refreshComboTrayAction(); });
    m_comboTrayEvent->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelTrayEvent, m_comboTrayEvent);
}

QLayout *IfacePage::setupTrayActionLayout()
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

void IfacePage::setupConfirmationsBox()
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

void IfacePage::updateTheme()
{
    const auto colorScheme = IniUser::colorSchemeByName(iniUser()->theme());
    m_comboTheme->setCurrentIndex(colorScheme);
}

void IfacePage::updateStyle()
{
    const auto style = iniUser()->style();
    m_comboStyle->setCurrentText(style);
}
