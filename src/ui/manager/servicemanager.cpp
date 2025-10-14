#include "servicemanager.h"

#include <QCoreApplication>
#include <QHash>
#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version.h>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <control/controlmanager.h>
#include <fortglobal.h>
#include <util/osutil.h>
#include <util/service/service_types.h>
#include <util/startuputil.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("manager.service");

quint32 processControlPauseState(
        ServiceManager *serviceManager, quint32 /*code*/, quint32 /*eventType*/)
{
    if (!serviceManager->acceptPauseContinue())
        return 0;

    qCDebug(LC) << "Pause due service control";

    emit serviceManager->pauseRequested();

    return SERVICE_PAUSED;
}

quint32 processControlContinueState(
        ServiceManager *serviceManager, quint32 /*code*/, quint32 /*eventType*/)
{
    if (!serviceManager->acceptPauseContinue())
        return 0;

    qCDebug(LC) << "Continue due service control";

    emit serviceManager->continueRequested();

    return SERVICE_RUNNING;
}

quint32 processControlStopState(
        ServiceManager *serviceManager, quint32 /*code*/, quint32 /*eventType*/)
{
    return serviceManager->acceptStop() ? SERVICE_STOP_PENDING : 0;
}

quint32 processControlShutdownState(
        ServiceManager *serviceManager, quint32 code, quint32 /*eventType*/)
{
    const bool restarting = (code == ServiceControlStopRestarting);
    if (restarting || code == ServiceControlStopUninstall) {
        emit serviceManager->stopRestartingRequested(restarting);
    }

    return SERVICE_STOP_PENDING;
}

enum ServiceControlHandlerCode : qint8 {
    ControlHandlerNone = -1,
    ControlHandlerPause = 0,
    ControlHandlerContinue,
    ControlHandlerStop,
    ControlHandlerShutdown,
};

inline ServiceControlHandlerCode toControlHandlerCode(quint32 code)
{
    static const QHash<quint8, ServiceControlHandlerCode> controlHandlerCodes = {
        { SERVICE_CONTROL_PAUSE, ControlHandlerPause },
        { SERVICE_CONTROL_CONTINUE, ControlHandlerContinue },
        { SERVICE_CONTROL_STOP, ControlHandlerStop },
        { ServiceControlStop, ControlHandlerShutdown },
        { ServiceControlStopRestarting, ControlHandlerShutdown },
        { ServiceControlStopUninstall, ControlHandlerShutdown },
        { SERVICE_CONTROL_SHUTDOWN, ControlHandlerShutdown },
    };

    return controlHandlerCodes.value(code, ControlHandlerNone);
}

using controlHandler_func = quint32 (*)(
        ServiceManager *serviceManager, quint32 code, quint32 eventType);

static const controlHandler_func controlHandler_funcList[] = {
    &processControlPauseState, // ControlHandlerPause,
    &processControlContinueState, // ControlHandlerContinue,
    &processControlStopState, // ControlHandlerStop,
    &processControlShutdownState, // ControlHandlerShutdown,
};

inline quint32 processControlState(ServiceManager *serviceManager, quint32 code, quint32 eventType)
{
    const auto handlerCode = toControlHandlerCode(code);
    if (handlerCode == ControlHandlerNone)
        return 0;

    const controlHandler_func func = controlHandler_funcList[handlerCode];

    return func(serviceManager, code, eventType);
}

}

ServiceManager::ServiceManager(QObject *parent) : QObject(parent) { }

void ServiceManager::setUp()
{
    connect(qApp, &QObject::destroyed, [] { reportStatus(SERVICE_STOPPED); });

    setupControlManager();
    setupConfManager();
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
}

void ServiceManager::setupControlManager()
{
    auto controlManager = Fort::dependency<ControlManager>();

    connect(this, &ServiceManager::pauseRequested, controlManager, [controlManager] {
        controlManager->close();
        controlManager->closeAllClients();
    });
    connect(this, &ServiceManager::continueRequested, controlManager, &ControlManager::listen);
}

void ServiceManager::setupConfManager()
{
    auto confManager = Fort::dependency<ConfManager>();

    connect(confManager, &ConfManager::iniChanged, this, &ServiceManager::setupByConfIni);
}

void ServiceManager::setupByConfIni()
{
    setControlEnabled(!ini().noServiceControl());
}

const wchar_t *ServiceManager::serviceName() const
{
    return StartupUtil::serviceName();
}

void ServiceManager::processControl(quint32 code, quint32 eventType)
{
    const auto state = processControlState(this, code, eventType);

    if (state == SERVICE_STOP_PENDING) {
        OsUtil::quit("service control"); // it's threadsafe
    }

    reportStatus(state);
}

void ServiceManager::restart()
{
    connect(qApp, &QObject::destroyed, [] { OsUtil::startService(APP_BASE "Svc"); });

    OsUtil::quit("service required restart");
}
