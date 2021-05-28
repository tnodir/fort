#include <QApplication>

#ifdef USE_VISUAL_LEAK_DETECTOR
#    include <vld.h>
#endif

#include <breakpad/crashhandler.h>
#include <fort_version.h>

#include "control/controlmanager.h"
#include "driver/drivercommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/osutil.h"
#include "util/serviceworker.h"
#include "util/startuputil.h"

namespace {

#define FORT_ERROR_INSTANCE 1
#define FORT_ERROR_CONTROL  2

void uninstall()
{
    StartupUtil::setAutoRunMode(StartupUtil::StartupDisabled); // Remove auto-run
    StartupUtil::setServiceInstalled(false); // Uninstall service
    StartupUtil::setExplorerIntegrated(false); // Remove Windows Explorer integration
    DriverCommon::provUnregister(); // Unregister booted provider
}

void install(const char *arg)
{
    if (arg[0] == 'p') { // "portable"
        StartupUtil::setPortable(true);
    } else if (arg[0] == 's') { // "service"
        StartupUtil::setAutoRunMode(StartupUtil::StartupAllUsers);
        StartupUtil::setServiceInstalled(true);
    } else if (arg[0] == 'e') { // "explorer"
        StartupUtil::setExplorerIntegrated(true);
    }
}

bool processArgs(int argc, char *argv[])
{
    // Uninstall
    if (argc > 1 && !strcmp(argv[1], "-u")) {
        uninstall();
        return true;
    }

    // Install
    if (argc > 2 && !strcmp(argv[1], "-i")) {
        install(argv[2]);
        return true;
    }

    return false;
}

void setupCrashHandler(CrashHandler &crashHandler, const FortSettings &settings)
{
    const QString dumpPath = settings.logsPath();
    const QString fileNamePrefix = "crash_fort_" + (settings.isService() ? "svc_" : QString());
    const QString fileNameSuffix = ".dmp";
    constexpr int CRASH_KEEP_FILES = 7;

    FileUtil::removeOldFiles(dumpPath, fileNamePrefix, fileNameSuffix, CRASH_KEEP_FILES);

    crashHandler.setFileNamePrefix(fileNamePrefix);
    crashHandler.setFileNameSuffix(fileNameSuffix);
    crashHandler.install(dumpPath);
}

}

int main(int argc, char *argv[])
{
    if (processArgs(argc, argv))
        return 0;

    CrashHandler crashHandler;

    // Process global settings required before QApplication costruction
    FortSettings settings;
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

    // Setup Crash Handler
#ifndef QT_DEBUG
    setupCrashHandler(crashHandler, settings);
#endif

    // Setup Control Manager
    ControlManager controlManager(&settings);

    if (controlManager.isCommandClient()) // Send control command to running instance
        return controlManager.postCommand() ? 0 : FORT_ERROR_CONTROL;

    // Setup Fort Manager
    FortManager::setupResources();

    FortManager fortManager(&settings, &envManager, &controlManager);

    // Check running instance
    if (!fortManager.checkRunningInstance())
        return FORT_ERROR_INSTANCE;

    fortManager.initialize();

    if (settings.isService()) {
        ServiceWorker::run();
    } else {
        fortManager.show();
    }

    return QApplication::exec();
}
