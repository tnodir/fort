#include <QApplication>

#include "../common/version.h"
#include "driver/drivermanager.h"
#include "fortcommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "util/osutil.h"

int main(int argc, char *argv[])
{
    OsUtil::createGlobalMutex(APP_NAME);

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION_STR);
    app.setApplicationDisplayName(APP_NAME " v" APP_VERSION_STR);

    FortSettings fortSettings(qApp->arguments());

    // Register booted provider and exit
    if (fortSettings.boot()) {
        FortCommon::provUnregister();
        return FortCommon::provRegister(true);
    }

    FortManager fortManager(&fortSettings);

    // Error: Cannot open the driver device
    if (!fortManager.driverManager()->isDeviceOpened())
        return 1;

    fortManager.showTrayIcon();

    return app.exec();
}
