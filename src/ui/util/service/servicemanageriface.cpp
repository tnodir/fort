#include "servicemanageriface.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <dbt.h>

/* Service global structure */
static struct
{
    HDEVNOTIFY hdevnotify;
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

void ServiceManagerIface::registerDeviceNotification()
{
    if (g_service.hdevnotify)
        return;

    DEV_BROADCAST_DEVICEINTERFACE filter;
    ZeroMemory(&filter, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
    filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    g_service.hdevnotify = RegisterDeviceNotification(g_service.hstatus, &filter,
            DEVICE_NOTIFY_SERVICE_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
}

void ServiceManagerIface::unregisterDeviceNotification()
{
    if (!g_service.hdevnotify)
        return;

    UnregisterDeviceNotification(g_service.hdevnotify);

    g_service.hdevnotify = nullptr;
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

    if (g_service.hstatus) {
        SetServiceStatus(g_service.hstatus, &g_service.status);
    }
}

bool ServiceManagerIface::isDeviceEvent(quint32 eventType)
{
    switch (eventType) {
    case DBT_DEVICEARRIVAL:
    case DBT_DEVICEREMOVECOMPLETE:
        return true;
    default:
        return false;
    }
}
