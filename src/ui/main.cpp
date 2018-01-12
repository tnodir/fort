#include <QApplication>

#include "../common/version.h"
#include "driver/drivermanager.h"
#include "fortcommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "util/osutil.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION_STR);
    app.setApplicationDisplayName(APP_NAME " v" APP_VERSION_STR);

    FortSettings fortSettings(qApp->arguments());

    // Unregister booted provider and exit
    if (fortSettings.hasProvBoot()) {
        FortCommon::provUnregister();
        return 0;
    }

    // To check running instance
    OsUtil::createGlobalMutex(APP_NAME);

    FortManager fortManager(&fortSettings);

    // Error: Cannot open the driver device
    if (!fortManager.driverManager()->isDeviceOpened())
        return 1;

    fortManager.showTrayIcon();

    return app.exec();
}
