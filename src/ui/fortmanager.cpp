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
#include <fortglobal.h>
#include <fortsettings.h>
#include <hostinfo/hostinfocache.h>
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
#include <rpc/statconnmanagerrpc.h>
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

using namespace Fort;

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
    ioc->setService(new StatConnManager(settings->statConnFilePath()));
    ioc->setService(new AskPendingManager());
    ioc->setService(new AutoUpdateManager(settings->updatePath()));
    ioc->setService(new DriverManager());
    ioc->setService(new AppInfoManager(settings->cacheFilePath()));
    ioc->setService(new LogManager());
    ioc->setService(new ServiceInfoManager());
    ioc->setService(new TaskManager());
    ioc->setService(new DbErrorManager());
}

inline void setupClientServices(IocContainer *ioc, const FortSettings *settings)
{
    ioc->setService<ConfManager>(new ConfManagerRpc(settings->confFilePath()));
    ioc->setService<ConfAppManager>(new ConfAppManagerRpc());
    ioc->setService<ConfRuleManager>(new ConfRuleManagerRpc());
    ioc->setService<ConfZoneManager>(new ConfZoneManagerRpc());
    ioc->setService<QuotaManager>(new QuotaManagerRpc());
    ioc->setService<StatManager>(new StatManagerRpc(settings->statFilePath()));
    ioc->setService<StatConnManager>(new StatConnManagerRpc(settings->statConnFilePath()));
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
        if (!controlManager()->postCommand(Control::CommandHome, { "show" })) {
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
    setupConfRuleManager();
    setupQuotaManager();
    setupTaskManager();
    setupServiceInfoManager();

    checkReinstallDriver();
    checkStartService();

    if (!setupDriver()) {
        checkDriverAccess();
    }

    loadConf();
}

void FortManager::setupThreadPool()
{
    QThreadPool::globalInstance()->setMaxThreadCount(16);
}

void FortManager::setupLogger()
{
    Logger *logger = Logger::instance();

    const auto settings = Fort::settings();

    logger->setIsPortable(settings->isPortable());
    logger->setIsService(settings->isService());
    logger->setHasService(settings->hasService());
    logger->setPath(settings->logsPath());
    logger->setForceDebug(settings->forceDebug());
}

void FortManager::updateLogger()
{
    const auto &ini = Fort::ini();

    Logger *logger = Logger::instance();

    logger->setDebug(ini.logDebug());
    logger->setConsole(ini.logConsole());
}

void FortManager::createManagers()
{
    IocContainer *ioc = IoCPinned();

    const auto settings = Fort::settings();

    setupServices(ioc, settings);

    if (settings->isMaster()) {
        // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
        FileUtil::removeFile(settings->statFilePath() + "-block");
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
    const bool hasService = settings()->hasService();

    if (hasService) {
        StartupUtil::stopService();
    } else {
        closeDriver();
    }

    driverManager()->reinstallDriver();

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

    driverManager()->uninstallDriver();

    return true;
}

bool FortManager::setupDriver()
{
    auto driverManager = Fort::driverManager();

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

    driverManager()->closeDevice();

    QCoreApplication::sendPostedEvents(this);
}

void FortManager::checkRemoveDriver()
{
    const auto settings = Fort::settings();

    if (!canInstallDriver(settings))
        return;

    if (settings->isService() || StartupUtil::isServiceInstalled())
        return;

    removeDriver();
}

void FortManager::checkReinstallDriver()
{
    const auto settings = Fort::settings();

    if (!settings->hasService() && canInstallDriver(settings)) {
        driverManager()->checkReinstallDriver();
    }
}

void FortManager::checkStartService()
{
    const auto settings = Fort::settings();

    if (settings->isMaster() || StartupUtil::isServiceRunning())
        return;

    const bool canStartService = settings->canStartService();
    const bool isAdmin = settings->isUserAdmin();

    if (canStartService && isAdmin) {
        StartupUtil::startService();
    }
}

void FortManager::checkDriverAccess()
{
    const auto settings = Fort::settings();

    const bool hasService = settings->hasService();
    const bool isAdmin = settings->isUserAdmin();

    if (!(hasService || isAdmin)) {
        QMessageBox::warning(nullptr, QString(),
                tr("Run Fort Firewall as Administrator or install its Windows Service"
                   " to access the Driver!"));
    }
}

void FortManager::setupEnvManager()
{
    auto envManager = Fort::envManager();

    connect(nativeEventFilter(), &NativeEventFilter::environmentChanged, envManager,
            &EnvManager::onEnvironmentChanged);

    connect(envManager, &EnvManager::environmentUpdated, this, [&] { updateDriverConf(); });
}

void FortManager::setupConfManager()
{
    auto confManager = Fort::confManager();

    connect(confManager, &ConfManager::imported, windowManager(), &WindowManager::closeAllWindows);

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

void FortManager::setupConfRuleManager()
{
    connect(confRuleManager(), &ConfRuleManager::ruleRemoved, this,
            [&](int /*ruleId*/, int appRulesCount) {
                if (appRulesCount > 0) {
                    updateDriverConf(); // Update all apps
                }
            });
}

void FortManager::setupQuotaManager()
{
    connect(quotaManager(), &QuotaManager::alert, this, [&](qint8 alertType) {
        windowManager()->showInfoBox(QuotaManager::alertTypeText(alertType), tr("Quota Alert"));
    });
}

void FortManager::setupTaskManager()
{
    auto taskManager = Fort::taskManager();

    connect(taskManager, &TaskManager::appVersionDownloaded, this, [&](const QString &version) {
        windowManager()->showTrayMessage(
                tr("New version v%1 available!").arg(version), tray::MessageNewVersion);
    });

    connect(taskManager, &TaskManager::zonesDownloaded, this, [&](const QStringList &zoneNames) {
        windowManager()->showTrayMessage(
                tr("Zone Addresses Updated: %1.").arg(zoneNames.join(", ")), tray::MessageZones);
    });

    connect(taskManager, &TaskManager::zonesUpdated, confZoneManager(),
            &ConfZoneManager::updateDriverZones);

    connect(taskManager, &TaskManager::taskDoubleClicked, this, [&](qint8 taskType) {
        auto windowManager = Fort::windowManager();

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
    connect(serviceInfoManager(), &ServiceInfoManager::servicesStarted, confManager(),
            &ConfManager::updateDriverServices);
}

void FortManager::processRestartRequired(const QString &info)
{
    qCDebug(LC) << "Restart required:" << info;

    if (settings()->isService()) {
        serviceManager()->restart();
    } else {
        OsUtil::restart();
    }
}

void FortManager::loadConf()
{
    auto confManager = Fort::confManager();
    const auto settings = Fort::settings();

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
    auto confManager = Fort::confManager();

    if (!confManager->validateDriver())
        return false;

    // Services
    confManager->updateServices();

    // Zones
    {
        auto zd = taskManager()->taskInfoZoneDownloader();

        confZoneManager()->updateDriverZones(
                zd->dataZonesMask(), zd->enabledMask(), zd->dataSize(), zd->zonesData());
    }

    // Rules
    {
        confRuleManager()->updateDriverRules();
    }

    return true;
}

void FortManager::updateDriverConf(bool onlyFlags)
{
    auto confAppManager = Fort::confAppManager();

    if (!confAppManager->canUpdateDriverConf())
        return;

    updateLogManager(false);

    const bool ok = confAppManager->updateDriverConf(onlyFlags);
    if (ok) {
        updateStatManager(conf());
    }

    updateLogManager(true);
}

void FortManager::updateLogManager(bool active)
{
    logManager()->setActive(active);
}

void FortManager::updateStatManager(FirewallConf *conf)
{
    statManager()->setConf(conf);
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
