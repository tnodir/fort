#include "servicemanager.h"

#include <QCoreApplication>
#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version.h>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <control/controlmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/service/service_types.h>
#include <util/startuputil.h>

namespace {

const QLoggingCategory LC("manager.service");

}

ServiceManager::ServiceManager(QObject *parent) : QObject(parent) { }

void ServiceManager::setUp()
{
    connect(qApp, &QObject::destroyed, [] { reportStatus(SERVICE_STOPPED); });

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
    auto controlManager = IoCDependency<ControlManager>();

    connect(this, &ServiceManager::pauseRequested, controlManager, [controlManager] {
        controlManager->close();
        controlManager->closeAllClients();
    });
    connect(this, &ServiceManager::continueRequested, controlManager, &ControlManager::listen);
}

void ServiceManager::setupConfManager()
{
    auto confManager = IoCDependency<ConfManager>();

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
    const auto state = processControlState(code, eventType);

    if (state == SERVICE_STOP_PENDING) {
        OsUtil::quit("service control"); // it's threadsafe
    }

    reportStatus(state);
}

quint32 ServiceManager::processControlState(quint32 code, quint32 eventType)
{
    switch (code) {
    case SERVICE_CONTROL_PAUSE: {
        return processControlPauseState();
    } break;

    case SERVICE_CONTROL_CONTINUE: {
        return processControlContinueState();
    } break;

    case SERVICE_CONTROL_STOP: {
        return processControlStopState();
    } break;

    case ServiceControlStop:
    case ServiceControlStopRestarting:
    case ServiceControlStopUninstall:
    case SERVICE_CONTROL_SHUTDOWN: {
        return processControlShutdownState(code);
    } break;

    case SERVICE_CONTROL_DEVICEEVENT: {
        processControlDeviceEvent(eventType);
    } break;
    }

    return 0;
}

quint32 ServiceManager::processControlPauseState()
{
    if (!acceptPauseContinue())
        return 0;

    qCDebug(LC) << "Pause due service control";

    emit pauseRequested();

    return SERVICE_PAUSED;
}

quint32 ServiceManager::processControlContinueState()
{
    if (!acceptPauseContinue())
        return 0;

    qCDebug(LC) << "Continue due service control";

    emit continueRequested();

    return SERVICE_RUNNING;
}

quint32 ServiceManager::processControlStopState()
{
    return acceptStop() ? SERVICE_STOP_PENDING : 0;
}

quint32 ServiceManager::processControlShutdownState(quint32 code)
{
    const bool restarting = (code == ServiceControlStopRestarting);
    if (restarting || code == ServiceControlStopUninstall) {
        emit stopRestartingRequested(restarting);
    }

    return SERVICE_STOP_PENDING;
}

void ServiceManager::processControlDeviceEvent(quint32 eventType)
{
    if (isDeviceEvent(eventType)) {
        emit driveListChanged();
    }
}

void ServiceManager::restart()
{
    connect(qApp, &QObject::destroyed, [] { OsUtil::startService(APP_BASE "Svc"); });

    OsUtil::quit("service required restart");
}
