#include "homepage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <driver/drivermanager.h>
#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/startuputil.h>

namespace {

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

}

HomePage::HomePage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void HomePage::onRetranslateUi()
{
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

    m_btUninstallPortable->setText(tr("Uninstall"));

    m_cbExplorerMenu->setText(tr("Windows Explorer integration"));
    m_cbSoundsPanel->setText(tr("Control Panel: Sounds integration"));
    m_labelAutoRun->setText(tr("Auto-run:"));
    retranslateComboAutoRun();
}

void HomePage::retranslateDriverMessage()
{
    const auto text = driverManager()->isDeviceError()
            ? driverManager()->errorMessage()
            : (driverManager()->isDeviceOpened() ? tr("Driver Installed")
                                                 : tr("Driver Not Installed"));

    m_labelDriverMessage->setText(text);
}

void HomePage::retranslateServiceMessage()
{
    const auto text = hasService() ? tr("Service Installed") : tr("Service Not Installed");

    m_labelServiceMessage->setText(text);
}

void HomePage::retranslateComboAutoRun()
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
    {
        auto comboModel = qobject_cast<QStandardItemModel *>(m_comboAutoRun->model());
        Q_ASSERT(comboModel);

        auto item = comboModel->item(StartupUtil::StartupAllUsers);
        Q_ASSERT(item);

        item->setEnabled(false);
    }
}

void HomePage::setupUi()
{
    // Column
    auto colLayout = setupColumn();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addStretch();
    layout->addLayout(colLayout);
    layout->addStretch();

    this->setLayout(layout);
}

QLayout *HomePage::setupColumn()
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

void HomePage::setupDriverBox()
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

QLayout *HomePage::setupDriverLabelLayout()
{
    // Message
    m_labelDriverMessage = ControlUtil::createLabel();
    m_labelDriverMessage->setWordWrap(true);
    m_labelDriverMessage->setFont(GuiUtil::fontBold());

    // Icon
    setupDriverIcon();

    auto layout = new QHBoxLayout();
    layout->setSpacing(4);
    layout->addStretch();
    layout->addWidget(m_iconDriver, 0, Qt::AlignTop);
    layout->addWidget(m_labelDriverMessage);
    layout->addStretch();

    return layout;
}

void HomePage::setupDriverIcon()
{
    const QSize iconSize(16, 16);
    m_iconDriver = ControlUtil::createIconLabel(":/icons/server_components.png", iconSize);

    const auto refreshDriverInfo = [&] {
        m_iconDriver->setEnabled(driverManager()->isDeviceOpened());
    };

    refreshDriverInfo();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverInfo);
    connect(driverManager(), &DriverManager::errorCodeChanged, this,
            &HomePage::retranslateDriverMessage);
}

QLayout *HomePage::setupDriverButtonsLayout()
{
    m_btInstallDriver = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox([&] { fortManager()->installDriver(); },
                tr("Are you sure to reinstall the Driver?"));
    });
    m_btRemoveDriver = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] { fortManager()->removeDriver(); }, tr("Are you sure to remove the Driver?"));
    });

    auto layout = new QHBoxLayout();
    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(m_btInstallDriver);
    layout->addWidget(m_btRemoveDriver);
    layout->addStretch();

    return layout;
}

void HomePage::setupServiceBox()
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

QLayout *HomePage::setupServiceLabelLayout()
{
    // Message
    m_labelServiceMessage = ControlUtil::createLabel();
    m_labelServiceMessage->setWordWrap(true);
    m_labelServiceMessage->setFont(GuiUtil::fontBold());

    // Icon
    setupServiceIcon();

    auto layout = new QHBoxLayout();
    layout->setSpacing(4);
    layout->addStretch();
    layout->addWidget(m_iconService, 0, Qt::AlignTop);
    layout->addWidget(m_labelServiceMessage);
    layout->addStretch();

    return layout;
}

void HomePage::setupServiceIcon()
{
    const QSize iconSize(16, 16);
    m_iconService = ControlUtil::createIconLabel(":/icons/widgets.png", iconSize);
}

QLayout *HomePage::setupServiceButtonsLayout()
{
    auto layout = new QHBoxLayout();
    layout->setSpacing(10);

    m_btInstallService = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] { setServiceInstalled(true); }, tr("Are you sure to install the Service?"));
    });
    m_btRemoveService = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] { setServiceInstalled(false); }, tr("Are you sure to remove the Service?"));
    });

    layout->addStretch();
    layout->addWidget(m_btInstallService);
    layout->addWidget(m_btRemoveService);
    layout->addStretch();

    return layout;
}

void HomePage::setupPortableBox()
{
    // Buttons Row
    auto buttonsLayout = setupPortableButtonsLayout();

    m_gbPortable = createGroupBox();

    auto layout = groupBoxLayout(m_gbPortable);
    layout->addLayout(buttonsLayout);
}

QLayout *HomePage::setupPortableButtonsLayout()
{
    m_btUninstallPortable = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] {
                    FortManager::uninstall();
                    fortManager()->removeDriver();

                    updateHasService();
                },
                tr("Are you sure to uninstall the Fort Firewall?"));
    });

    auto layout = new QHBoxLayout();
    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(m_btUninstallPortable);
    layout->addStretch();

    return layout;
}

void HomePage::setupIntegrationBox()
{
    auto integrationLayout = setupIntegrationLayout();

    m_gbIntegration = createGroupBox();

    auto layout = groupBoxLayout(m_gbIntegration);
    layout->addLayout(integrationLayout);
}

QLayout *HomePage::setupIntegrationLayout()
{
    m_cbExplorerMenu = ControlUtil::createCheckBox(StartupUtil::isExplorerIntegrated(),
            [&](bool checked) { StartupUtil::setExplorerIntegrated(checked); });

    m_cbSoundsPanel = ControlUtil::createCheckBox(StartupUtil::isSoundsPanelIntegrated(),
            [&](bool checked) { StartupUtil::setSoundsPanelIntegrated(checked); });

    // Auto Run Row
    auto autoRunLayout = setupAutoRunLayout();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbExplorerMenu);
    layout->addWidget(m_cbSoundsPanel);
    layout->addLayout(autoRunLayout);

    return layout;
}

QLayout *HomePage::setupAutoRunLayout()
{
    m_labelAutoRun = ControlUtil::createLabel();

    m_comboAutoRun = ControlUtil::createComboBox(QStringList(),
            [&](int index) { StartupUtil::setAutoRunMode(index, settings()->defaultLanguage()); });
    m_comboAutoRun->setFixedWidth(200);

    return ControlUtil::createRowLayout(m_labelAutoRun, m_comboAutoRun);
}

void HomePage::updateIsUserAdmin()
{
    if (settings()->isUserAdmin())
        return;

    m_btInstallDriver->setEnabled(false);
    m_btRemoveDriver->setEnabled(false);

    m_btInstallService->setEnabled(false);
    m_btRemoveService->setEnabled(false);

    m_btUninstallPortable->setEnabled(false);
}

void HomePage::updateIsPortable()
{
    if (settings()->isPortable())
        return;

    m_gbPortable->setVisible(false);
}

void HomePage::updateHasService()
{
    m_hasService = StartupUtil::isServiceInstalled();

    m_btRemoveDriver->setVisible(!hasService());
    retranslateServiceMessage();
    m_iconService->setEnabled(hasService());
    m_btInstallService->setVisible(!hasService());
    m_btRemoveService->setVisible(hasService());
}

void HomePage::setServiceInstalled(bool install)
{
    StartupUtil::setServiceInstalled(install);

    updateHasService();

    windowManager()->processRestartRequired(tr("Windows Service installation changed."));
}
