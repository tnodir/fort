#include "optionspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/labelspincombo.h>
#include <form/opt/optionscontroller.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

namespace {

const std::array timedOptionSecondsValues = { 10, 0, 20, 60, 60 * 2, 60 * 5, 60 * 10,
    60 * 60 /* 1 hour */, 60 * 60 * 24 /* 1 day */ };

void updateComboBox(
        QComboBox *c, const QStringList &names, const QStringList &iconPaths, int currentIndex)
{
    int index = 0;
    for (const QString &name : names) {
        const QString &iconPath = iconPaths.at(index);

        c->setItemText(index, name);
        c->setItemData(index, name, Qt::ToolTipRole);
        c->setItemIcon(index, IconCache::icon(iconPath));

        ++index;
    }

    c->setCurrentIndex(currentIndex);
}

}

OptionsPage::OptionsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void OptionsPage::onResetToDefault()
{
    m_cbFilterEnabled->setChecked(true);
    m_comboBlockTraffic->setCurrentIndex(0);
    m_comboFilterMode->setCurrentIndex(0);
    m_cbGroupBlocked->setChecked(true);
    m_lscFilterOffSeconds->spinBox()->setValue(DEFAULT_FILTER_OFF_SECONDS);
    m_lscAutoLearnSeconds->spinBox()->setValue(DEFAULT_AUTO_LEARN_SECONDS);

    m_cbBootFilter->setChecked(false);
    m_cbStealthMode->setChecked(false);
    m_cbNoServiceControl->setChecked(false);
    m_cbCheckPasswordOnUninstall->setChecked(false);
    m_cbPassword->setChecked(false);

    m_cbLogApp->setChecked(true);
    m_cbRemoveLearntApps->setChecked(false);
    m_cbPurgeOnMounted->setChecked(false);

    m_cbUpdateKeepCurrentVersion->setChecked(false);
    m_cbUpdateAutoDownload->setChecked(false);
    m_cbUpdateAutoInstall->setChecked(false);

    m_cbLogDebug->setChecked(false);
    m_cbLogConsole->setChecked(false);
    m_cbTraceEvents->setChecked(false);
}

void OptionsPage::onAboutToSave()
{
    onAboutToSave_filterMode();
    onAboutToSave_password();
}

void OptionsPage::onAboutToSave_filterMode()
{
    if (filterModeEdited())
        return;

    const auto oldFilterMode = confManager()->conf()->filterMode();

    if (conf()->filterMode() != oldFilterMode) {
        // Reset to the new value, when auto-switched from Auto-learn
        conf()->setFilterMode(oldFilterMode);
    }
}

void OptionsPage::onAboutToSave_password()
{
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
    // Filter Mode
    setFilterModeEdited(false);

    // Password
    setPasswordEdited(false);
    retranslateEditPassword();
}

void OptionsPage::onRetranslateUi()
{
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbProtection->setTitle(tr("Self Protection"));
    m_gbUpdate->setTitle(tr("Auto Update"));
    m_gbLogs->setTitle(tr("Logs"));
    m_gbProg->setTitle(tr("Programs"));

    m_cbFilterEnabled->setText(tr("Filter Enabled"));

    m_labelBlockTraffic->setText(tr("Block Traffic:"));
    retranslateComboBlockTraffic();

    m_labelFilterMode->setText(tr("Filter Mode:"));
    retranslateComboFilterMode();

    m_cbGroupBlocked->setText(tr("Block traffic for disabled App Groups"));

    m_lscFilterOffSeconds->label()->setText(tr("Filter Off seconds:"));
    m_lscAutoLearnSeconds->label()->setText(tr("Auto-learn seconds:"));
    retranslateTimedOptions();

    m_cbBootFilter->setText(tr("Block traffic when Fort Firewall is not running"));
    m_cbStealthMode->setText(tr("Stealth mode (Prevent port scanning)"));
    m_cbNoServiceControl->setText(tr("Disable Service controls"));
    m_cbCheckPasswordOnUninstall->setText(tr("Check password on Uninstall"));

    m_cbPassword->setText(tr("Password:"));
    retranslateEditPassword();

    m_btPasswordLock->setText(tr("Lock the password (unlocked till \"%1\")")
                    .arg(settings()->passwordUnlockedTillText()));

    m_cbLogApp->setText(tr("Collect New Programs"));
    m_cbRemoveLearntApps->setText(tr("Remove alerted programs on Auto-Learn Off"));
    m_cbPurgeOnMounted->setText(tr("Purge Obsolete only on mounted drives"));

    m_cbUpdateKeepCurrentVersion->setText(tr("Keep current version"));
    m_cbUpdateKeepCurrentVersion->setToolTip(
            tr("Don't delete the last downloaded installer after installation"));
    m_cbUpdateAutoDownload->setText(tr("Auto-download new version"));
    m_cbUpdateAutoInstall->setText(tr("Auto-install after download"));

    m_cbLogDebug->setText(tr("Log debug messages"));
    m_cbLogConsole->setText(tr("Show log messages in console"));
    m_cbTraceEvents->setText(tr("Trace Driver Events"));
}

void OptionsPage::retranslateComboBlockTraffic()
{
    updateComboBox(m_comboBlockTraffic, FirewallConf::blockTrafficNames(),
            FirewallConf::blockTrafficIconPaths(), conf()->blockTrafficIndex());
}

void OptionsPage::retranslateComboFilterMode()
{
    updateComboBox(m_comboFilterMode, FirewallConf::filterModeNames(),
            FirewallConf::filterModeIconPaths(), conf()->filterMode());
}

void OptionsPage::retranslateTimedOptions()
{
    const QStringList list = { tr("Custom"), tr("Disabled"), tr("20 seconds"), tr("1 minute"),
        tr("2 minutes"), tr("5 minutes"), tr("10 minutes"), tr("1 hour"), tr("1 day") };

    const auto suffix = tr(" second(s)");

    m_lscFilterOffSeconds->setNames(list);
    m_lscFilterOffSeconds->spinBox()->setSuffix(suffix);

    m_lscAutoLearnSeconds->setNames(list);
    m_lscAutoLearnSeconds->spinBox()->setSuffix(suffix);
}

void OptionsPage::retranslateEditPassword()
{
    m_editPassword->setPlaceholderText(
            settings()->hasPassword() ? tr("Installed") : tr("Not Installed"));
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
    // Traffic Group Box
    setupTrafficBox();

    // Protection Group Box
    setupProtectionBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbTraffic);
    layout->addWidget(m_gbProtection);
    layout->addStretch();

    return layout;
}

QLayout *OptionsPage::setupColumn2()
{
    // Programs Group Box
    setupProgBox();

    // Update Group Box
    setupUpdateBox();

    // Logs Group Box
    setupLogsBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbProg);
    layout->addWidget(m_gbUpdate);
    layout->addWidget(m_gbLogs);
    layout->addStretch();

    return layout;
}

void OptionsPage::setupTrafficBox()
{
    // Filter Enabled
    m_cbFilterEnabled = ControlUtil::createCheckBox(conf()->filterEnabled(), [&](bool checked) {
        conf()->setFilterEnabled(checked);
        ctrl()->setFlagsEdited();
    });

    // Block Traffic
    auto blockTrafficLayout = setupBlockTrafficLayout();

    // Filter Mode
    auto filterModeLayout = setupFilterModeLayout();

    // Group Blocked
    m_cbGroupBlocked = ControlUtil::createCheckBox(conf()->groupBlocked(), [&](bool checked) {
        conf()->setGroupBlocked(checked);
        ctrl()->setFlagsEdited();
    });

    // Timed Options
    setupFilterOffSeconds();
    setupAutoLearnSeconds();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbFilterEnabled);
    layout->addLayout(blockTrafficLayout);
    layout->addLayout(filterModeLayout);
    layout->addWidget(m_cbGroupBlocked);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addWidget(m_lscFilterOffSeconds);
    layout->addWidget(m_lscAutoLearnSeconds);

    m_gbTraffic = new QGroupBox();
    m_gbTraffic->setLayout(layout);
}

QLayout *OptionsPage::setupBlockTrafficLayout()
{
    m_labelBlockTraffic = ControlUtil::createLabel();

    m_comboBlockTraffic =
            ControlUtil::createComboBox(FirewallConf::blockTrafficNames(), [&](int index) {
                if (conf()->blockTrafficIndex() != index) {
                    conf()->setBlockTrafficIndex(index);
                    ctrl()->setFlagsEdited();
                }
            });
    m_comboBlockTraffic->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelBlockTraffic, m_comboBlockTraffic);
}

QLayout *OptionsPage::setupFilterModeLayout()
{
    m_labelFilterMode = ControlUtil::createLabel();

    m_comboFilterMode =
            ControlUtil::createComboBox(FirewallConf::filterModeNames(), [&](int index) {
                if (conf()->filterMode() != index) {
                    conf()->setFilterMode(FirewallConf::FilterMode(index));
                    ctrl()->setFlagsEdited();
                    setFilterModeEdited(true);
                }
            });
    m_comboFilterMode->setFixedWidth(200);

    // TODO: Implement Ask to Connect
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_comboFilterMode->model());
    QStandardItem *item = model->item(1);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    return ControlUtil::createRowLayout(m_labelFilterMode, m_comboFilterMode);
}

void OptionsPage::setupFilterOffSeconds()
{
    m_lscFilterOffSeconds = new LabelSpinCombo();
    m_lscFilterOffSeconds->spinBox()->setRange(0, 99999); // ~27.7 hours
    m_lscFilterOffSeconds->setValues(timedOptionSecondsValues);

    m_lscFilterOffSeconds->spinBox()->setValue(ini()->filterOffSeconds());

    connect(m_lscFilterOffSeconds->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (ini()->filterOffSeconds() != value) {
                    ini()->setFilterOffSeconds(value);
                    ctrl()->setIniEdited();
                }
            });
}

void OptionsPage::setupAutoLearnSeconds()
{
    m_lscAutoLearnSeconds = new LabelSpinCombo();
    m_lscAutoLearnSeconds->spinBox()->setRange(0, 99999); // ~27.7 hours
    m_lscAutoLearnSeconds->setValues(timedOptionSecondsValues);

    m_lscAutoLearnSeconds->spinBox()->setValue(ini()->autoLearnSeconds());

    connect(m_lscAutoLearnSeconds->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (ini()->autoLearnSeconds() != value) {
                    ini()->setAutoLearnSeconds(value);
                    ctrl()->setIniEdited();
                }
            });
}

void OptionsPage::setupProtectionBox()
{
    m_cbBootFilter = ControlUtil::createCheckBox(conf()->bootFilter(), [&](bool checked) {
        conf()->setBootFilter(checked);
        ctrl()->setFlagsEdited();
    });

    m_cbStealthMode = ControlUtil::createCheckBox(conf()->stealthMode(), [&](bool checked) {
        conf()->setStealthMode(checked);
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

    if (!settings()->hasMasterAdmin()) {
        m_cbCheckPasswordOnUninstall->setEnabled(false);
    }

    // Password Row
    auto passwordLayout = setupPasswordLayout();
    setupPasswordLock();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbBootFilter);
    layout->addWidget(m_cbStealthMode);
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

        ini()->setHasPassword(checked);
        ctrl()->setIniEdited();
        setPasswordEdited(true);
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
    setupLogApp();

    m_cbRemoveLearntApps =
            ControlUtil::createCheckBox(ini()->progRemoveLearntApps(), [&](bool checked) {
                if (ini()->progRemoveLearntApps() != checked) {
                    ini()->setProgRemoveLearntApps(checked);
                    ctrl()->setIniEdited();
                }
            });

    m_cbPurgeOnMounted =
            ControlUtil::createCheckBox(ini()->progPurgeOnMounted(), [&](bool checked) {
                if (ini()->progPurgeOnMounted() != checked) {
                    ini()->setProgPurgeOnMounted(checked);
                    ctrl()->setIniEdited();
                }
            });

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbLogApp, ControlUtil::createSeparator(),
            m_cbRemoveLearntApps, m_cbPurgeOnMounted });

    m_gbProg = new QGroupBox();
    m_gbProg->setLayout(layout);
}

void OptionsPage::setupLogApp()
{
    m_cbLogApp = ControlUtil::createCheckBox(conf()->logApp(), [&](bool checked) {
        if (conf()->logApp() != checked) {
            conf()->setLogApp(checked);
            ctrl()->setFlagsEdited();
        }
    });

    m_cbLogApp->setFont(GuiUtil::fontBold());
}

void OptionsPage::setupUpdateBox()
{
    m_cbUpdateKeepCurrentVersion =
            ControlUtil::createCheckBox(ini()->updateKeepCurrentVersion(), [&](bool checked) {
                ini()->setUpdateKeepCurrentVersion(checked);
                ctrl()->setIniEdited();
            });

    m_cbUpdateAutoDownload =
            ControlUtil::createCheckBox(ini()->updateAutoDownload(), [&](bool checked) {
                ini()->setUpdateAutoDownload(checked);
                ctrl()->setIniEdited();
            });

    m_cbUpdateAutoInstall =
            ControlUtil::createCheckBox(ini()->updateAutoInstall(), [&](bool checked) {
                ini()->setUpdateAutoInstall(checked);
                ctrl()->setIniEdited();
            });

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbUpdateKeepCurrentVersion, m_cbUpdateAutoDownload, m_cbUpdateAutoInstall });

    m_gbUpdate = new QGroupBox();
    m_gbUpdate->setLayout(layout);
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

    m_cbTraceEvents = ControlUtil::createCheckBox(conf()->traceEvents(), [&](bool checked) {
        conf()->setTraceEvents(checked);
        ctrl()->setFlagsEdited();
    });

    auto layout =
            ControlUtil::createVLayoutByWidgets({ m_cbLogDebug, m_cbLogConsole, m_cbTraceEvents });

    m_gbLogs = new QGroupBox();
    m_gbLogs->setLayout(layout);
}
