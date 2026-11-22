#include "installationwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <driver/drivermanager.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/startuputil.h>

using namespace Fort;

namespace {

inline constexpr auto iconSize = QSize(16, 16);

QGroupBox *createGroupBox()
{
    auto c = new QGroupBox();
    c->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    c->setMinimumWidth(200);

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    c->setLayout(layout);

    return c;
}

QVBoxLayout *groupBoxLayout(QGroupBox *c)
{
    return static_cast<QVBoxLayout *>(c->layout());
}

QLabel *createMessageLabel()
{
    auto c = ControlUtil::createLabel();
    c->setWordWrap(true);
    c->setFont(GuiUtil::fontBold());
    return c;
}

QLayout *createMessageLabelLayout(QLabel *icon, QLabel *label)
{
    auto l = new QHBoxLayout();
    l->setSpacing(4);
    l->addWidget(icon, 0, Qt::AlignTop);
    l->addWidget(label, 1);
    return l;
}

}

InstallationWindow::InstallationWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &InstallationWindow::retranslateUi);
    retranslateUi();

    setupFormWindow(iniUser());
}

QString InstallationWindow::iniGroup() const
{
    return IniUser::installationWindowGroup();
}

void InstallationWindow::saveWindowState(bool /*wasVisible*/)
{
    saveIniWindowGeometry();

    confManager()->saveIniUser();
}

void InstallationWindow::restoreWindowState()
{
    restoreIniWindowGeometry(QSize(450, 500));
}

void InstallationWindow::retranslateUi()
{
    this->unsetLocale();

    m_gbDriver->setTitle(tr("Driver"));
    m_gbService->setTitle(tr("Windows Service"));
    m_gbPortable->setTitle(tr("Portable"));
    m_gbIntegration->setTitle(tr("Integration"));

    retranslateDriverMessage();
    m_btInstallDriver->setText(tr("Reinstall"));
    m_btRemoveDriver->setText(tr("Remove"));

    retranslateServiceMessage();
    m_btInstallService->setText(tr("Install"));
    m_btInstallService->setToolTip(tr("Run Fort Firewall as a Service in background"));
    m_btRemoveService->setText(tr("Remove"));

    retranslatePortableMessage();
    m_btUninstallPortable->setText(tr("Uninstall"));

    m_cbExplorerMenu->setText(tr("Windows Explorer integration"));
    m_cbSoundsPanel->setText(tr("Control Panel: Sounds integration"));
    m_labelAutoRun->setText(tr("Auto-run:"));
    retranslateComboAutoRun();

    this->setWindowTitle(tr("Installation"));
}

void InstallationWindow::retranslateDriverMessage()
{
    const auto text = driverManager()->isDeviceError()
            ? driverManager()->errorMessage()
            : (driverManager()->isDeviceOpened() ? tr("Driver Installed")
                                                 : tr("Driver Not Installed"));

    m_labelDriverMessage->setText(text);
}

void InstallationWindow::retranslateServiceMessage()
{
    const auto text = hasService() ? tr("Service Installed") : tr("Service Not Installed");

    m_labelServiceMessage->setText(text);
}

void InstallationWindow::retranslatePortableMessage()
{
    m_labelPortableMessage->setText(tr("Portable Installed"));
}

void InstallationWindow::retranslateComboAutoRun()
{
    const QStringList list = { tr("Disabled"), tr("For current user"), tr("For all users") };

    int currentIndex = m_comboAutoRun->currentIndex();
    if (currentIndex < 0) {
        currentIndex = StartupUtil::autoRunMode();
    }

    ControlUtil::setComboBoxTexts(m_comboAutoRun, list, currentIndex);

    // Disable some items if user is not an administrator
    if (settings()->isUserAdmin())
        return;

    // Disable the combobox
    if (currentIndex >= StartupUtil::StartupAllUsers) {
        m_comboAutoRun->setEnabled(false);
        return;
    }

    // Disable the item for all users
    ControlUtil::setComboBoxItemsEnabled(
            m_comboAutoRun, /*enabled=*/false, StartupUtil::StartupAllUsers);
}

void InstallationWindow::setupUi()
{
    // Column
    auto colLayout = setupColumn();

    auto frameLayout = new QHBoxLayout();
    frameLayout->addStretch();
    frameLayout->addLayout(colLayout);
    frameLayout->addStretch();

    // Main layout
    auto layout = ControlUtil::createVLayout();
    layout->addWidget(ControlUtil::wrapToScrollArea(ControlUtil::wrapToWidget(frameLayout)));

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(300, 200);
}

QLayout *InstallationWindow::setupColumn()
{
    // Driver Group Box
    setupDriverBox();

    // Service Group Box
    setupServiceBox();

    // Portable Group Box
    setupPortableBox();

    // Integration Group Box
    setupIntegrationBox();

    updateIsUserAdmin();
    updateIsPortable();
    updateHasService();

    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_gbDriver, m_gbService, m_gbPortable, m_gbIntegration, /* stretch */ nullptr });
    layout->setSpacing(10);

    return layout;
}

void InstallationWindow::setupDriverBox()
{
    // Label Row
    auto labelLayout = setupDriverLabelLayout();

    // Buttons Row
    auto buttonsLayout = setupDriverButtonsLayout();

    m_gbDriver = createGroupBox();

    auto layout = groupBoxLayout(m_gbDriver);
    layout->addLayout(labelLayout);
    layout->addLayout(buttonsLayout);
}

QLayout *InstallationWindow::setupDriverLabelLayout()
{
    // Icon
    setupDriverIcon();

    // Message
    m_labelDriverMessage = createMessageLabel();

    return createMessageLabelLayout(m_iconDriver, m_labelDriverMessage);
}

void InstallationWindow::setupDriverIcon()
{
    m_iconDriver = ControlUtil::createIconLabel(":/icons/server_components.png", iconSize);

    const auto refreshDriverInfo = [&] {
        m_iconDriver->setEnabled(driverManager()->isDeviceOpened());
    };

    refreshDriverInfo();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverInfo);
    connect(driverManager(), &DriverManager::errorCodeChanged, this,
            &InstallationWindow::retranslateDriverMessage);
}

QLayout *InstallationWindow::setupDriverButtonsLayout()
{
    m_btInstallDriver = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                this, [&] { fortManager()->installDriver(); },
                tr("Are you sure to reinstall the Driver?"));
    });
    m_btInstallDriver->setObjectName("btInstallDriver");
    m_btRemoveDriver = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                this, [&] { fortManager()->removeDriver(); },
                tr("Are you sure to remove the Driver?"));
    });
    m_btRemoveDriver->setObjectName("btRemoveDriver");

    auto layout = new QHBoxLayout();
    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(m_btInstallDriver);
    layout->addWidget(m_btRemoveDriver);
    layout->addStretch();

    return layout;
}

void InstallationWindow::setupServiceBox()
{
    // Label Row
    auto labelLayout = setupServiceLabelLayout();

    // Buttons Row
    auto buttonsLayout = setupServiceButtonsLayout();

    m_gbService = createGroupBox();

    auto layout = groupBoxLayout(m_gbService);
    layout->addLayout(labelLayout);
    layout->addLayout(buttonsLayout);
}

QLayout *InstallationWindow::setupServiceLabelLayout()
{
    // Icon
    m_iconService = ControlUtil::createIconLabel(":/icons/widgets.png", iconSize);

    // Message
    m_labelServiceMessage = createMessageLabel();

    return createMessageLabelLayout(m_iconService, m_labelServiceMessage);
}

QLayout *InstallationWindow::setupServiceButtonsLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(10);

    m_btInstallService = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                this, [&] { setServiceInstalled(true); },
                tr("Are you sure to install the Service?"));
    });
    m_btInstallService->setObjectName("btInstallService");
    m_btRemoveService = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                this, [&] { setServiceInstalled(false); },
                tr("Are you sure to remove the Service?"));
    });
    m_btRemoveService->setObjectName("btRemoveService");

    layout->addStretch();
    layout->addWidget(m_btInstallService);
    layout->addWidget(m_btRemoveService);
    layout->addStretch();

    return layout;
}

void InstallationWindow::setupPortableBox()
{
    // Label Row
    auto labelLayout = setupPortableLabelLayout();

    // Buttons Row
    auto buttonsLayout = setupPortableButtonsLayout();

    m_gbPortable = createGroupBox();

    auto layout = groupBoxLayout(m_gbPortable);
    layout->addLayout(labelLayout);
    layout->addLayout(buttonsLayout);
}

QLayout *InstallationWindow::setupPortableLabelLayout()
{
    // Icon
    m_iconPortable = ControlUtil::createIconLabel(":/icons/luggage_brown.png", iconSize);

    // Message
    m_labelPortableMessage = createMessageLabel();

    return createMessageLabelLayout(m_iconPortable, m_labelPortableMessage);
}

QLayout *InstallationWindow::setupPortableButtonsLayout()
{
    m_btUninstallPortable = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                this,
                [&] {
                    FortManager::uninstall();
                    fortManager()->removeDriver();

                    updateHasService();
                },
                tr("Are you sure to uninstall the Fort Firewall?"));
    });
    m_btUninstallPortable->setObjectName("btUninstallPortable");

    auto layout = new QHBoxLayout();
    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(m_btUninstallPortable);
    layout->addStretch();

    return layout;
}

void InstallationWindow::setupIntegrationBox()
{
    auto integrationLayout = setupIntegrationLayout();

    m_gbIntegration = createGroupBox();

    auto layout = groupBoxLayout(m_gbIntegration);
    layout->addLayout(integrationLayout);
}

QLayout *InstallationWindow::setupIntegrationLayout()
{
    m_cbExplorerMenu = ControlUtil::createCheckBox(StartupUtil::isExplorerIntegrated(),
            [&](bool checked) { StartupUtil::setExplorerIntegrated(checked); });
    m_cbExplorerMenu->setObjectName("cbExplorerMenu");

    m_cbSoundsPanel = ControlUtil::createCheckBox(StartupUtil::isSoundsPanelIntegrated(),
            [&](bool checked) { StartupUtil::setSoundsPanelIntegrated(checked); });
    m_cbSoundsPanel->setObjectName("cbSoundsPanel");

    // Auto Run Row
    auto autoRunLayout = setupAutoRunLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbExplorerMenu);
    layout->addWidget(m_cbSoundsPanel);
    layout->addLayout(autoRunLayout);

    return layout;
}

QLayout *InstallationWindow::setupAutoRunLayout()
{
    m_labelAutoRun = ControlUtil::createLabel();

    m_comboAutoRun = ControlUtil::createComboBox(QStringList(),
            [&](int index) { StartupUtil::setAutoRunMode(index, settings()->defaultLanguage()); });
    m_comboAutoRun->setObjectName("comboAutoRun");
    m_comboAutoRun->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelAutoRun, m_comboAutoRun);
}

void InstallationWindow::updateIsUserAdmin()
{
    if (settings()->isUserAdmin())
        return;

    m_btInstallDriver->setEnabled(false);
    m_btRemoveDriver->setEnabled(false);

    m_btUninstallPortable->setEnabled(false);
}

void InstallationWindow::updateIsPortable()
{
    if (settings()->isPortable())
        return;

    m_gbPortable->setVisible(false);
}

void InstallationWindow::updateHasService()
{
    m_hasService = StartupUtil::isServiceInstalled();

    m_btRemoveDriver->setVisible(!hasService());
    retranslateServiceMessage();
    m_iconService->setEnabled(hasService());
    m_btInstallService->setVisible(!hasService());
    m_btRemoveService->setVisible(hasService());
}

void InstallationWindow::setServiceInstalled(bool install)
{
    if (!fortManager()->setServiceInstalled(install))
        return;

    updateHasService();

    windowManager()->processRestartRequired(tr("Windows Service installation changed."));
}
