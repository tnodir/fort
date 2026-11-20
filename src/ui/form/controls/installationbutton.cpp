#include "installationbutton.h"

#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

using namespace Fort;

InstallationButton::InstallationButton(QWidget *parent) : ToolButton(parent)
{
    setupUi();

    connect(this, &QToolButton::clicked, this, &InstallationButton::showInstallationWindow);
}

void InstallationButton::showInstallationWindow()
{
    windowManager()->showInstallationWindow();
}

void InstallationButton::setupUi()
{
    setupIcon();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &InstallationButton::retranslateUi);
    retranslateUi();
}

void InstallationButton::setupIcon()
{
    const QIcon icon = IconCache::icon(":/icons/server_components.png");

    // The button stays clickable: the gray icon is the icon's disabled mode shown as the normal one
    // The tool bar's icon size isn't known yet: take the icon's own one, it's scaled when drawn
    const QSize size = icon.actualSize(QSize(256, 256));

    m_iconGray.addPixmap(icon.pixmap(size, QIcon::Disabled));
    m_iconNormal = icon;

    connect(driverManager(), &DriverManager::isDeviceOpenedChanged, this,
            &InstallationButton::updateIcon);
    updateIcon();
}

void InstallationButton::updateIcon()
{
    setIcon(driverManager()->isDeviceOpened() ? m_iconNormal : m_iconGray);
}

void InstallationButton::retranslateUi()
{
    setToolTip(tr("Installation"));
}
