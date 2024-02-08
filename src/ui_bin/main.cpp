#include <QApplication>

#ifdef USE_VISUAL_LEAK_DETECTOR
#    include <vld.h>
#endif

#include <breakpad/crashhandler.h>
#include <fort_version.h>

#include <control/controlmanager.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/envmanager.h>
#include <manager/servicemanager.h>
#include <manager/windowmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/service/serviceworker.h>
#include <util/startuputil.h>

namespace {

enum FortError {
    FortErrorInstance = 1,
    FortErrorControl,
    FortErrorService,
};

bool processArgs(int argc, char *argv[], int &rc)
{
    if (argc <= 1)
        return false;

    const char *arg = argv[1];
    if (arg[0] != '-')
        return false;

    const char param = arg[1];

    // Stop
    if (param == 's') {
        rc = StartupUtil::stopService() ? 0 : FortErrorService;
        return true;
    }

    // Uninstall
    if (param == 'u') {
        FortManager::uninstall();
        return true;
    }

    // Install
    if (param == 'i' && argc > 2) {
        FortManager::install(argv[2]);
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

    crashHandler.setFileNamePrefix(fileNamePrefix + APP_VERSION_STR + APP_VERSION_BUILD_STR + '_');
    crashHandler.setFileNameSuffix(fileNameSuffix);
    crashHandler.install(dumpPath);
}

}

int main(int argc, char *argv[])
{
    // Process global settings required before QApplication costruction
    FortSettings settings;
    settings.setupGlobal();

    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION_STR);
    QApplication::setApplicationDisplayName(QLatin1String(APP_NAME) + ' ' + APP_VERSION_STR
            + APP_VERSION_BUILD_STR
            + (settings.isPortable() ? QLatin1String(" Portable") : QString()));

    // Process (un)install arguments
    {
        int rc = 0;
        if (processArgs(argc, argv, rc))
            return rc;
    }

    EnvManager envManager;

    // Initialize settings from command line arguments
    settings.initialize(QCoreApplication::arguments(), &envManager);

    // Setup Crash Handler
#ifndef QT_DEBUG
    CrashHandler crashHandler;
    setupCrashHandler(crashHandler, settings);
#endif

    // Setup IoC Container
    IocContainer ioc;
    ioc.pinToThread();
    ioc.set<FortSettings>(settings);
    ioc.set<EnvManager>(envManager);

    // Setup Control Manager
    ControlManager controlManager;
    ioc.setService<ControlManager>(controlManager);

    if (controlManager.isCommandClient()) {
        // Send control command to running instance
        return controlManager.processCommandClient() ? 0 : FortErrorControl;
    }

    // Setup Fort Manager
    FortManager::setupResources();

    FortManager fortManager;
    ioc.set<FortManager>(fortManager);

    // Check running instance
    if (!fortManager.checkRunningInstance(settings.isService()))
        return FortErrorInstance;

    fortManager.initialize();

    if (settings.isService()) {
        ServiceWorker::run(IoC<ServiceManager>());
    } else {
        IoC<WindowManager>()->initialize();
    }

    return QApplication::exec();
}
