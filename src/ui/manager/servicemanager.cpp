#include "servicemanager.h"

#include <QCoreApplication>
#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <control/controlmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/startuputil.h>

namespace {

const QLoggingCategory LC("manager.service");

}

ServiceManager::ServiceManager(QObject *parent) : QObject(parent) { }

ServiceManager::~ServiceManager() { }

void ServiceManager::setUp()
{
    setupControlManager();
    setupConfManager();
}

void ServiceManager::tearDown()
{
    unregisterDeviceNotification();
}

void ServiceManager::setControlEnabled(bool v)
{
    if (m_controlEnabled == v)
        return;

    m_controlEnabled = v;

    setupAcceptedControls();
    reportStatus();
}

void ServiceManager::initialize(qintptr hstatus)
{
    ServiceManagerIface::initialize(hstatus);

    registerDeviceNotification();
}

void ServiceManager::setupControlManager()
{
    auto controlManager = IoC()->setUpDependency<ControlManager>();

    connect(this, &ServiceManager::pauseRequested, controlManager, [controlManager] {
        controlManager->close();
        controlManager->closeAllClients();
    });
    connect(this, &ServiceManager::continueRequested, controlManager, &ControlManager::listen);
}

void ServiceManager::setupConfManager()
{
    auto confManager = IoC()->setUpDependency<ConfManager>();

    connect(confManager, &ConfManager::iniChanged, this, &ServiceManager::setupByConf);
}

void ServiceManager::setupByConf(const IniOptions &ini)
{
    setControlEnabled(!ini.noServiceControl());
}

const wchar_t *ServiceManager::serviceName() const
{
    return StartupUtil::serviceName();
}

void ServiceManager::processControl(quint32 code, quint32 eventType)
{
    DWORD state = SERVICE_RUNNING;

    switch (code) {
    case SERVICE_CONTROL_PAUSE: {
        if (!acceptPauseContinue())
            break;

        qCDebug(LC) << "Pause due service control";

        emit pauseRequested();

        state = SERVICE_PAUSED;
    } break;
    case SERVICE_CONTROL_CONTINUE: {
        if (!acceptPauseContinue())
            break;

        qCDebug(LC) << "Continue due service control";

        emit continueRequested();

        state = SERVICE_RUNNING;
    } break;
    case SERVICE_CONTROL_STOP:
        if (!acceptStop())
            break;

        Q_FALLTHROUGH();
    case FORT_SERVICE_CONTROL_UNINSTALL:
    case SERVICE_CONTROL_SHUTDOWN: {
        qCDebug(LC) << "Quit due service control";

        qApp->connect(qApp, &QObject::destroyed, [] { reportStatus(SERVICE_STOPPED); });
        QCoreApplication::quit(); // it's threadsafe

        state = SERVICE_STOP_PENDING;
    } break;
    case SERVICE_CONTROL_DEVICEEVENT: {
        if (isDeviceEvent(eventType)) {
            emit driveListChanged();
        }
    } break;
    }

    reportStatus(state);
}
