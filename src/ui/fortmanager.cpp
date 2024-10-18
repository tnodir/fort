#include "fortmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QThreadPool>

#include <fort_version.h>

#include <appinfo/appinfocache.h>
#include <conf/firewallconf.h>
#include <control/controlmanager.h>
#include <driver/drivercommon.h>
#include <form/dialog/passworddialog.h>
#include <fortsettings.h>
#include <hostinfo/hostinfocache.h>
#include <manager/drivelistmanager.h>
#include <manager/envmanager.h>
#include <manager/hotkeymanager.h>
#include <manager/logger.h>
#include <manager/nativeeventfilter.h>
#include <manager/servicemanager.h>
#include <manager/translationmanager.h>
#include <model/zonelistmodel.h>
#include <rpc/appinfomanagerrpc.h>
#include <rpc/askpendingmanagerrpc.h>
#include <rpc/autoupdatemanagerrpc.h>
#include <rpc/confappmanagerrpc.h>
#include <rpc/confmanagerrpc.h>
#include <rpc/confrulemanagerrpc.h>
#include <rpc/confzonemanagerrpc.h>
#include <rpc/dberrormanagerrpc.h>
#include <rpc/drivermanagerrpc.h>
#include <rpc/logmanagerrpc.h>
#include <rpc/quotamanagerrpc.h>
#include <rpc/rpcmanager.h>
#include <rpc/serviceinfomanagerrpc.h>
#include <rpc/statblockmanagerrpc.h>
#include <rpc/statmanagerrpc.h>
#include <rpc/taskmanagerrpc.h>
#include <rpc/windowmanagerfake.h>
#include <task/taskinfozonedownloader.h>
#include <user/usersettings.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/startuputil.h>

namespace {

const QLoggingCategory LC("fortManager");

bool canInstallDriver(FortSettings *settings)
{
    const bool canInstallDriver = (settings->canInstallDriver() || settings->isPortable());
    const bool isAdmin = settings->isUserAdmin();

    return (canInstallDriver && isAdmin);
}

inline void setupMasterServices(IocContainer *ioc, const FortSettings *settings)
{
    ioc->setService(new ConfManager(settings->confFilePath()));
    ioc->setService(new ConfAppManager());
    ioc->setService(new ConfRuleManager());
    ioc->setService(new ConfZoneManager());
    ioc->setService(new QuotaManager());
    ioc->setService(new StatManager(settings->statFilePath()));
    ioc->setService(new StatBlockManager(settings->statBlockFilePath()));
    ioc->setService(new AskPendingManager());
    ioc->setService(new AutoUpdateManager(settings->updatePath()));
    ioc->setService(new DriverManager());
    ioc->setService(new AppInfoManager(settings->cacheFilePath()));
    ioc->setService(new LogManager());
    ioc->setService(new ServiceInfoManager());
    ioc->setService(new TaskManager());
    ioc->setService(new DbErrorManager());

    // For Master only
    ioc->setService(new DriveListManager());
}

inline void setupClientServices(IocContainer *ioc, const FortSettings *settings)
{
    ioc->setService<ConfManager>(new ConfManagerRpc(settings->confFilePath()));
    ioc->setService<ConfAppManager>(new ConfAppManagerRpc());
    ioc->setService<ConfRuleManager>(new ConfRuleManagerRpc());
    ioc->setService<ConfZoneManager>(new ConfZoneManagerRpc());
    ioc->setService<QuotaManager>(new QuotaManagerRpc());
    ioc->setService<StatManager>(new StatManagerRpc(settings->statFilePath()));
    ioc->setService<StatBlockManager>(new StatBlockManagerRpc(settings->statBlockFilePath()));
    ioc->setService<AskPendingManager>(new AskPendingManagerRpc());
    ioc->setService<AutoUpdateManager>(new AutoUpdateManagerRpc(settings->updatePath()));
    ioc->setService<DriverManager>(new DriverManagerRpc());
    ioc->setService<AppInfoManager>(
            new AppInfoManagerRpc(settings->cacheFilePath(), settings->noCache()));
    ioc->setService<LogManager>(new LogManagerRpc());
    ioc->setService<ServiceInfoManager>(new ServiceInfoManagerRpc());
    ioc->setService<TaskManager>(new TaskManagerRpc());
    ioc->setService<DbErrorManager>(new DbErrorManagerRpc());
}

inline void setupServices(IocContainer *ioc, const FortSettings *settings)
{
    if (settings->isMaster()) {
        setupMasterServices(ioc, settings);
    } else {
        setupClientServices(ioc, settings);
    }

    if (settings->hasService()) {
        ioc->setService(new RpcManager());
    }

    if (settings->isService()) {
        ioc->setService<WindowManager>(new WindowManagerFake());

        // For Service only
        ioc->setService(new ServiceManager());
    } else {
        ioc->setService(new WindowManager());

        // For UI only
        ioc->setService(new HotKeyManager());
        ioc->setService(new UserSettings());
        ioc->setService(new TranslationManager());
    }

    ioc->setService(new NativeEventFilter());
    ioc->setService(new AppInfoCache());
    ioc->setService(new HostInfoCache());
    ioc->setService(new ZoneListModel());
}

}

FortManager::FortManager(QObject *parent) : QObject(parent) { }

FortManager::~FortManager()
{
    if (m_initialized) {
        closeDriver();

        emit aboutToDestroy();

        checkRemoveDriver();
    }

    deleteManagers();

    OsUtil::closeMutex(m_instanceMutex);
}

bool FortManager::checkRunningInstance(bool isService, bool isLaunch)
{
    bool isSingleInstance;
    m_instanceMutex =
            OsUtil::createMutex(isService ? "Global\\" APP_BASE : APP_BASE, isSingleInstance);
    if (isSingleInstance)
        return true;

    if (isService) {
        qCWarning(LC) << "Quit due Service is already running!";
    } else if (!isLaunch) {
        if (!IoC<ControlManager>()->postCommand(Control::CommandHome, { "show" })) {
            QMessageBox::warning(nullptr, QString(), tr("Application is already running!"));
        }
    }
    return false;
}

void FortManager::initialize()
{
    m_initialized = true;

    OsUtil::setCurrentThreadName("Main");

    setupThreadPool();
    setupLogger();

    createManagers();

    setupEnvManager();
    setupConfManager();
    setupQuotaManager();
    setupTaskManager();
    setupServiceInfoManager();

    checkReinstallDriver();
    checkStartService();

    setupDriver();
    loadConf();
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(16);
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    const auto settings = IoC<FortSettings>();

    logger->setIsPortable(settings->isPortable());
    logger->setIsService(settings->isService());
    logger->setHasService(settings->hasService());
    logger->setPath(settings->logsPath());
    logger->setForceDebug(settings->forceDebug());
}

void FortManager::updateLogger()
{
    const FirewallConf *conf = IoC<ConfManager>()->conf();

    Logger *logger = Logger::instance();

    logger->setDebug(conf->ini().logDebug());
    logger->setConsole(conf->ini().logConsole());
}

void FortManager::createManagers()
{
    IocContainer *ioc = IoCPinned();

    const auto settings = IoC<FortSettings>();

    setupServices(ioc, settings);

    if (settings->isMaster()) {
        // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
        FileUtil::copyFile(settings->statFilePath(), settings->statBlockFilePath());
    }

    ioc->setUpAll();

    if (settings->isMaster()) {
        OsUtil::endRestartClients();
    }
}

void FortManager::deleteManagers()
{
    IocContainer *ioc = IoCPinned();

    ioc->tearDownAll();
    ioc->autoDeleteAll();
}

void FortManager::install(const char *arg)
{
    if (!arg)
        return;

    switch (arg[0]) {
    case 'b': { // "boot-filter"
        DriverCommon::provRegister(/*bootFilter=*/true); // Register booted provider
    } break;
    case 'p': { // "portable"
        FortManager::setupPortableResource();
        StartupUtil::setPortable(true);
    } break;
    case 'a': { // "auto-run"
        StartupUtil::setAutoRunMode(StartupUtil::StartupAllUsers);
    } break;
    case 's': { // "service"
        StartupUtil::setServiceInstalled(true);
    } break;
    case 'e': { // "explorer"
        StartupUtil::setExplorerIntegrated(true);
    } break;
    }
}

void FortManager::uninstall(const char *arg)
{
    StartupUtil::setExplorerIntegrated(false); // Remove Windows Explorer integration
    if (arg && *arg == 'e') // "explorer"
        return;

    // COMPAT: Remove Global Windows Explorer integration
    StartupUtil::clearGlobalExplorerIntegrated();

    StartupUtil::stopService(ServiceControlStopUninstall); // Quit clients & Stop service
    StartupUtil::setServiceInstalled(false); // Uninstall service

    if (!arg) { // !"most"
        StartupUtil::setAutoRunMode(StartupUtil::StartupDisabled); // Remove auto-run

        DriverCommon::provUnregister(); // Unregister booted provider
    }
}

bool FortManager::installDriver()
{
    const bool hasService = IoC<FortSettings>()->hasService();

    if (hasService) {
        StartupUtil::stopService();
    } else {
        closeDriver();
    }

    IoC<DriverManager>()->reinstallDriver();

    if (hasService) {
        StartupUtil::setServiceInstalled();
    } else {
        // Re-open the driver device and initialize it
        if (setupDriver()) {
            updateDriverConf();
        }
    }

    return true;
}

bool FortManager::removeDriver()
{
    closeDriver();

    IoC<DriverManager>()->uninstallDriver();

    return true;
}

bool FortManager::setupDriver()
{
    auto driverManager = IoC<DriverManager>();

    if (!driverManager->openDevice())
        return false;

    if (!setupDriverConf()) {
        driverManager->closeDevice();
        return false;
    }

    return true;
}

void FortManager::closeDriver()
{
    updateLogManager(false);
    updateStatManager(nullptr);

    IoC<DriverManager>()->closeDevice();

    QCoreApplication::sendPostedEvents(this);
}

void FortManager::checkRemoveDriver()
{
    const auto settings = IoC<FortSettings>();

    if (!canInstallDriver(settings))
        return;

    if (settings->isService() || StartupUtil::isServiceInstalled())
        return;

    removeDriver();
}

void FortManager::checkReinstallDriver()
{
    const auto settings = IoC<FortSettings>();

    if (!settings->hasService() && canInstallDriver(settings)) {
        IoC<DriverManager>()->checkReinstallDriver();
    }
}

void FortManager::checkStartService()
{
    const auto settings = IoC<FortSettings>();

    if (settings->isMaster() || StartupUtil::isServiceRunning())
        return;

    const bool canStartService = settings->canStartService();
    const bool isAdmin = settings->isUserAdmin();

    if (canStartService && isAdmin) {
        StartupUtil::startService();
    }
}

void FortManager::setupEnvManager()
{
    auto envManager = IoC<EnvManager>();

    connect(IoC<NativeEventFilter>(), &NativeEventFilter::environmentChanged, envManager,
            &EnvManager::onEnvironmentChanged);

    connect(envManager, &EnvManager::environmentUpdated, this, [&] { updateDriverConf(); });
}

void FortManager::setupConfManager()
{
    auto confManager = IoC<ConfManager>();

    connect(confManager, &ConfManager::imported, IoC<WindowManager>(),
            &WindowManager::closeAllWindows);

    connect(confManager, &ConfManager::confChanged, this, [&](bool onlyFlags, uint editedFlags) {
        if ((editedFlags & FirewallConf::IniEdited) != 0) {
            updateLogger();
        }

        if (!onlyFlags || (editedFlags & FirewallConf::FlagsEdited) != 0) {
            updateDriverConf(onlyFlags);
        }
    });

    connect(confManager, &ConfManager::confPeriodsChanged, this,
            [&] { updateDriverConf(/*onlyFlags=*/true); });
}

void FortManager::setupQuotaManager()
{
    connect(IoC<QuotaManager>(), &QuotaManager::alert, this, [&](qint8 alertType) {
        IoC<WindowManager>()->showInfoBox(
                QuotaManager::alertTypeText(alertType), tr("Quota Alert"));
    });
}

void FortManager::setupTaskManager()
{
    auto taskManager = IoC<TaskManager>();

    connect(taskManager, &TaskManager::appVersionDownloaded, this, [&](const QString &version) {
        IoC<WindowManager>()->showTrayMessage(tr("New version v%1 available!").arg(version),
                WindowManager::TrayMessageNewVersion);
    });

    connect(taskManager, &TaskManager::zonesDownloaded, this, [&](const QStringList &zoneNames) {
        IoC<WindowManager>()->showTrayMessage(
                tr("Zone Addresses Updated: %1.").arg(zoneNames.join(", ")),
                WindowManager::TrayMessageZones);
    });

    connect(taskManager, &TaskManager::zonesUpdated, IoC<ConfZoneManager>(),
            &ConfZoneManager::updateDriverZones);

    connect(taskManager, &TaskManager::taskDoubleClicked, this, [&](qint8 taskType) {
        auto windowManager = IoC<WindowManager>();

        switch (taskType) {
        case TaskInfo::UpdateChecker: {
            windowManager->showHomeWindowAbout();
        } break;
        case TaskInfo::ZoneDownloader: {
            windowManager->showZonesWindow();
        } break;
        case TaskInfo::AppPurger: {
            windowManager->showProgramsWindow();
        } break;
        }
    });
}

void FortManager::setupServiceInfoManager()
{
    auto serviceInfoManager = IoC<ServiceInfoManager>();

    connect(serviceInfoManager, &ServiceInfoManager::servicesStarted, IoC<ConfManager>(),
            &ConfManager::updateDriverServices);
}

void FortManager::processRestartRequired(const QString &info)
{
    qCDebug(LC) << "Restart required:" << info;

    if (IoC<FortSettings>()->isService()) {
        IoC<ServiceManager>()->restart();
    } else {
        OsUtil::restart();
    }
}

void FortManager::loadConf()
{
    auto confManager = IoC<ConfManager>();
    const auto settings = IoC<FortSettings>();

    // Validate migration
    if (!confManager->checkCanMigrate(settings)) {
        exit(-1); // Exit the program
    }

    qCDebug(LC) << "Started as"
                << (settings->isService()                   ? "Service"
                                   : settings->hasService() ? "Client"
                                                            : "Program");

    confManager->load();
}

bool FortManager::setupDriverConf()
{
    auto confManager = IoC<ConfManager>();

    if (!confManager->validateDriver())
        return false;

    // Services
    confManager->updateServices();

    // Zones
    {
        auto zd = IoC<TaskManager>()->taskInfoZoneDownloader();

        IoC<ConfZoneManager>()->updateDriverZones(
                zd->dataZonesMask(), zd->enabledMask(), zd->dataSize(), zd->zonesData());
    }

    return true;
}

bool FortManager::updateDriverConf(bool onlyFlags)
{
    updateLogManager(false);

    const bool res = IoC<ConfAppManager>()->updateDriverConf(onlyFlags);
    if (res) {
        updateStatManager(IoC<ConfManager>()->conf());
    }

    updateLogManager(true);

    return res;
}

void FortManager::updateLogManager(bool active)
{
    IoC<LogManager>()->setActive(active);
}

void FortManager::updateStatManager(FirewallConf *conf)
{
    IoC<StatManager>()->setConf(conf);
}

void FortManager::setupPortableResource()
{
    Q_INIT_RESOURCE(fort_readme);
}

void FortManager::setupResources()
{
    Q_INIT_RESOURCE(appinfo_migrations);
    Q_INIT_RESOURCE(conf_migrations);
    Q_INIT_RESOURCE(conf_zone);
    Q_INIT_RESOURCE(stat_migrations);

    Q_INIT_RESOURCE(fort_icons);
    Q_INIT_RESOURCE(fort_scripts);
}
