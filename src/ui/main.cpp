#include <QApplication>

#include "../common/version.h"
#include "fortcommon.h"
#include "fortmanager.h"
#include "fortsettings.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION_STR);
    app.setApplicationDisplayName(APP_NAME " v" APP_VERSION_STR);

    FortManager fortManager;

    // Register booted provider and exit
    if (fortManager.fortSettings()->boot()) {
        FortCommon::provUnregister();
        return FortCommon::provRegister(true);
    }

    // Open the driver device
    if (!fortManager.setupDriver()) {
        return 1;
    }

    fortManager.showTrayIcon();

    return app.exec();
}
