#include "servicemanageriface.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

/* Service global structure */
static struct
{
    SERVICE_STATUS_HANDLE hstatus;
    SERVICE_STATUS status;
} g_service;

void ServiceManagerIface::initialize(qintptr hstatus)
{
    g_service.hstatus = SERVICE_STATUS_HANDLE(hstatus);

    g_service.status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_service.status.dwWin32ExitCode = NO_ERROR;
    g_service.status.dwServiceSpecificExitCode = 0;
    g_service.status.dwCheckPoint = 0;
    g_service.status.dwWaitHint = 1000;

    setupAcceptedControls();

    reportStatus(SERVICE_RUNNING);
}

void ServiceManagerIface::setupAcceptedControls()
{
    g_service.status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN
            | (acceptStop() ? SERVICE_ACCEPT_STOP : 0)
            | (acceptPauseContinue() ? SERVICE_ACCEPT_PAUSE_CONTINUE : 0);
}

void ServiceManagerIface::reportStatus(quint32 code)
{
    if (code != 0) {
        g_service.status.dwCurrentState = code;
    }

    if (g_service.hstatus != nullptr) {
        SetServiceStatus(g_service.hstatus, &g_service.status);
    }
}
