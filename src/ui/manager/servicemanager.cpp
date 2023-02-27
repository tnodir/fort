#include "servicemanager.h"

#include <QCoreApplication>
#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

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
    auto controlManager = IoC()->setUpDependency<ControlManager>();

    connect(this, &ServiceManager::pauseRequested, controlManager, [controlManager] {
        controlManager->close();
        controlManager->closeAllClients();
    });
    connect(this, &ServiceManager::continueRequested, controlManager, &ControlManager::listen);
}

const wchar_t *ServiceManager::serviceName() const
{
    return StartupUtil::serviceName();
}

void ServiceManager::processControl(quint32 code)
{
    DWORD state = SERVICE_RUNNING;

    switch (code) {
    case SERVICE_CONTROL_PAUSE: {
        qCDebug(LC) << "Pause due service control";

        emit pauseRequested();

        state = SERVICE_PAUSED;
    } break;
    case SERVICE_CONTROL_CONTINUE: {
        qCDebug(LC) << "Continue due service control";

        emit continueRequested();

        state = SERVICE_RUNNING;
    } break;
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN: {
        qCDebug(LC) << "Quit due service control";

        qApp->connect(qApp, &QObject::destroyed, [] { reportStatus(SERVICE_STOPPED); });
        QCoreApplication::quit(); // it's threadsafe

        state = SERVICE_STOP_PENDING;
    } break;
    }

    reportStatus(state);
}
