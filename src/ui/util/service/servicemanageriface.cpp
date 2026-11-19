#include "servicemanageriface.h"

#include <QMutex>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

// The period of the start's check points must be shorter than the wait hint
inline constexpr DWORD startPendingWaitHint = 3000;
inline constexpr DWORD runningWaitHint = 1000;

/* Service global structure: the status is reported by the start's thread, the control
 * handler's thread and the others */
struct
{
    QMutex mutex;
    SERVICE_STATUS_HANDLE hstatus;
    SERVICE_STATUS status;
} g_service;

void reportStatusLocked()
{
    if (g_service.hstatus) {
        SetServiceStatus(g_service.hstatus, &g_service.status);
    }
}

void setupAcceptedControlsLocked(const ServiceManagerIface &manager)
{
    // A pending service accepts no controls
    if (g_service.status.dwCurrentState == SERVICE_START_PENDING) {
        g_service.status.dwControlsAccepted = 0;
        return;
    }

    g_service.status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN
            | (manager.acceptStop() ? SERVICE_ACCEPT_STOP : 0)
            | (manager.acceptPauseContinue() ? SERVICE_ACCEPT_PAUSE_CONTINUE : 0);
}

void reportStateLocked(
        const ServiceManagerIface &manager, DWORD state, DWORD checkPoint, DWORD waitHint)
{
    g_service.status.dwCurrentState = state;
    g_service.status.dwCheckPoint = checkPoint;
    g_service.status.dwWaitHint = waitHint;

    setupAcceptedControlsLocked(manager);

    reportStatusLocked();
}

}

void ServiceManagerIface::initialize(qintptr hstatus)
{
    QMutexLocker locker(&g_service.mutex);

    g_service.hstatus = SERVICE_STATUS_HANDLE(hstatus);

    g_service.status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_service.status.dwWin32ExitCode = NO_ERROR;
    g_service.status.dwServiceSpecificExitCode = 0;

    reportStateLocked(*this, SERVICE_START_PENDING, /*checkPoint=*/1, startPendingWaitHint);
}

void ServiceManagerIface::reportStartPending()
{
    QMutexLocker locker(&g_service.mutex);

    ++g_service.status.dwCheckPoint;

    reportStatusLocked();
}

void ServiceManagerIface::reportRunning()
{
    QMutexLocker locker(&g_service.mutex);

    reportStateLocked(*this, SERVICE_RUNNING, /*checkPoint=*/0, runningWaitHint);
}

void ServiceManagerIface::setupAcceptedControls()
{
    QMutexLocker locker(&g_service.mutex);

    setupAcceptedControlsLocked(*this);
}

void ServiceManagerIface::reportStatus(quint32 code)
{
    QMutexLocker locker(&g_service.mutex);

    if (code != 0) {
        g_service.status.dwCurrentState = code;
    }

    reportStatusLocked();
}
