#include "homepage.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <driver/drivermanager.h>
#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

HomePage::HomePage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void HomePage::onRetranslateUi()
{
    m_gbDriver->setTitle(tr("Driver"));
    m_gbPortable->setTitle(tr("Portable"));

    retranslateDriverMessage();
    m_btInstallDriver->setText(tr("Reinstall"));
    m_btRemoveDriver->setText(tr("Remove"));

    m_btUninstallPortable->setText(tr("Uninstall"));
}

void HomePage::retranslateDriverMessage()
{
    const auto &text = driverManager()->isDeviceError()
            ? driverManager()->errorMessage()
            : (driverManager()->isDeviceOpened() ? tr("Installed") : tr("Not Installed"));

    m_labelDriverMessage->setText(text);
}

void HomePage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Driver Group Box
    setupDriverBox();
    layout->addWidget(m_gbDriver, 0, Qt::AlignHCenter);

    // Portable Group Box
    setupPortableBox();
    layout->addWidget(m_gbPortable, 0, Qt::AlignHCenter);

    layout->addStretch();

    this->setLayout(layout);
}

void HomePage::setupDriverBox()
{
    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);

    // Label Row
    auto labelLayout = new QHBoxLayout();
    labelLayout->setSpacing(4);
    colLayout->addLayout(labelLayout);

    m_labelDriverMessage = ControlUtil::createLabel();
    m_labelDriverMessage->setWordWrap(true);
    m_labelDriverMessage->setFont(GuiUtil::fontBold());

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
        windowManager()->showConfirmBox([&] { fortManager()->installDriver(); },
                tr("Are you sure to reinstall the Driver?"));
    });
    m_btRemoveDriver = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] { fortManager()->removeDriver(); }, tr("Are you sure to remove the Driver?"));
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

    m_gbDriver = new QGroupBox();
    m_gbDriver->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_gbDriver->setMinimumWidth(200);
    m_gbDriver->setLayout(colLayout);
}

void HomePage::setupDriverIcon()
{
    m_iconDriver = ControlUtil::createLabel();
    m_iconDriver->setScaledContents(true);
    m_iconDriver->setMaximumSize(16, 16);
    m_iconDriver->setPixmap(IconCache::file(":/icons/server_components.png"));

    const auto refreshDriverInfo = [&] {
        m_iconDriver->setEnabled(driverManager()->isDeviceOpened());
        retranslateDriverMessage();
    };

    refreshDriverInfo();

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this, refreshDriverInfo);
    connect(driverManager(), &DriverManager::errorCodeChanged, this,
            &HomePage::retranslateDriverMessage);
}

void HomePage::setupPortableBox()
{
    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);

    // Buttons Row
    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);
    colLayout->addLayout(buttonsLayout);

    m_btUninstallPortable = ControlUtil::createButton(QString(), [&] {
        windowManager()->showConfirmBox(
                [&] {
                    FortManager::uninstall();
                    fortManager()->removeDriver();
                },
                tr("Are you sure to uninstall the Fort Firewall?"));
    });

    if (!settings()->isUserAdmin()) {
        m_btUninstallPortable->setEnabled(false);
    }

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_btUninstallPortable);
    buttonsLayout->addStretch();

    m_gbPortable = new QGroupBox();
    m_gbPortable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_gbPortable->setMinimumWidth(200);
    m_gbPortable->setLayout(colLayout);

    m_gbPortable->setVisible(settings()->isPortable());
}
