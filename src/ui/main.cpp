#include <QApplication>
#include <QMessageBox>

#include "../common/version.h"
#include "control/controlmanager.h"
#include "control/controlworker.h"
#include "driver/drivermanager.h"
#include "fortcommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "util/osutil.h"

#define FORT_ERROR_INSTANCE 1
#define FORT_ERROR_DEVICE   2
#define FORT_ERROR_CONTROL  3

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION_STR);
    QApplication::setApplicationDisplayName(APP_NAME " v" APP_VERSION_STR);

    FortSettings fortSettings(qApp->arguments());

    // Unregister booted provider and exit
    if (fortSettings.hasProvBoot()) {
        FortCommon::provUnregister();
        return 0;
    }

    ControlManager controlManager(QApplication::applicationName(),
                                  fortSettings.controlPath());

    // Send control request to running instance
    if (controlManager.isClient()) {
        return controlManager.post(fortSettings.args())
                ? 0 : FORT_ERROR_CONTROL;
    }

    // Check running instance
    if (!OsUtil::createGlobalMutex(APP_NAME)) {
        QMessageBox::critical(nullptr, QString(),
                              "Application is already running!");
        return FORT_ERROR_INSTANCE;
    }

    FortManager fortManager(&fortSettings);

    // Error: Cannot open the driver device
    if (!fortManager.driverManager()->isDeviceOpened()) {
        QMessageBox::critical(nullptr, QString(),
                              "Cannot open the driver device!");
        return FORT_ERROR_DEVICE;
    }

    fortManager.showTrayIcon();

    // Process control requests from clients
    if (!controlManager.listen(&fortManager)) {
        return FORT_ERROR_CONTROL;
    }

    return QApplication::exec();
}
