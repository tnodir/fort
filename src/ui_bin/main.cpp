#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QStyleFactory>

#ifdef USE_VISUAL_LEAK_DETECTOR
#    include <vld.h>
#endif

#include <fort_version.h>

#include "control/controlmanager.h"
#include "driver/drivercommon.h"
#include "fortmanager.h"
#include "fortsettings.h"
#include "util/envmanager.h"
#include "util/osutil.h"
#include "util/serviceworker.h"
#include "util/startuputil.h"

#define FORT_ERROR_INSTANCE 1
#define FORT_ERROR_CONTROL  2

static void uninstall()
{
    StartupUtil::setStartupMode(StartupUtil::StartupDisabled); // Remove auto-run and service
    StartupUtil::integrateExplorer(false); // Remove Windows Explorer integration
    DriverCommon::provUnregister(); // Unregister booted provider
}

static void setupResources()
{
    Q_INIT_RESOURCE(appinfo_migrations);
    Q_INIT_RESOURCE(conf_migrations);
    Q_INIT_RESOURCE(conf_zone);
    Q_INIT_RESOURCE(stat_migrations);

    Q_INIT_RESOURCE(fort_icons);
}

static void setupAppStyle()
{
    const auto fusionStyle = QStyleFactory::create("Fusion");
    QApplication::setStyle(fusionStyle);
    QApplication::setPalette(fusionStyle->standardPalette());
}

int main(int argc, char *argv[])
{
    // Uninstall
    if (argc > 1 && !strcmp(argv[1], "-u")) {
        uninstall();
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
    if (controlManager.isCommandClient()) {
        return controlManager.postCommand() ? 0 : FORT_ERROR_CONTROL;
    }

    setupResources();

    FortManager fortManager(&settings, &envManager, &controlManager);

    // Check running instance
    if (!fortManager.checkRunningInstance())
        return FORT_ERROR_INSTANCE;

    fortManager.initialize();

    if (settings.isService()) {
        ServiceWorker::run();
    } else {
        setupAppStyle(); // Style & Palette

        fortManager.show();
    }

    return QApplication::exec();
}
