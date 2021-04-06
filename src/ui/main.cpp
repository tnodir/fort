#include <QApplication>
#include <QMessageBox>

#ifdef USE_VISUAL_LEAK_DETECTOR
#    include <vld.h>
#endif

#include <fort_version.h>

#include "control/controlmanager.h"
#include "fortcommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "service/servicemanager.h"
#include "util/envmanager.h"
#include "util/osutil.h"
#include "util/startuputil.h"

#define FORT_ERROR_INSTANCE 1
#define FORT_ERROR_CONTROL  2

int main(int argc, char *argv[])
{
    // Uninstall: Unregister booted provider, startup entries and exit
    if (argc > 1 && !strcmp(argv[1], "-u")) {
        StartupUtil::setStartupMode(StartupUtil::StartupDisabled);
        FortCommon::provUnregister();
        return 0;
    }

    // Process global settings required before QApplication costruction
    FortSettings settings;
    settings.setHasService(StartupUtil::isServiceInstalled());
    settings.setupGlobal();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION_STR);
    QApplication::setApplicationDisplayName(APP_NAME " v" APP_VERSION_STR);

    EnvManager envManager;

    // Initialize settings from command line arguments
    settings.initialize(QCoreApplication::arguments(), &envManager);

    ControlManager controlManager(&settings);

    // Send control command to running instance
    if (controlManager.isClient()) {
        return controlManager.postCommand() ? 0 : FORT_ERROR_CONTROL;
    }

    FortManager fortManager(&settings, &envManager);

    // Check running instance
    if (!fortManager.checkRunningInstance())
        return FORT_ERROR_INSTANCE;

    fortManager.initialize();

    if (settings.isService()) {
        ServiceManager::runService();
    } else {
        fortManager.show();
    }

    // Process control commands from clients
    if (!controlManager.listen(&fortManager))
        return FORT_ERROR_CONTROL;

    return QApplication::exec();
}
