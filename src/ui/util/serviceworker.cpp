#include "serviceworker.h"

#include <QCoreApplication>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "startuputil.h"

namespace {

/* Service global structure */
static struct
{
    SERVICE_STATUS status;
    SERVICE_STATUS_HANDLE hstatus;
} g_service;

void reportServiceStatus(DWORD state)
{
    g_service.status.dwCurrentState = state;
    SetServiceStatus(g_service.hstatus, &g_service.status);
}

void WINAPI serviceCtrlHandler(DWORD code)
{
    switch (code) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        qApp->connect(qApp, &QObject::destroyed, [] { reportServiceStatus(SERVICE_STOPPED); });
        QCoreApplication::quit(); // it's threadsafe

        reportServiceStatus(SERVICE_STOP_PENDING);
        break;
    }
}

void WINAPI serviceMain(DWORD argc, wchar_t *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    g_service.hstatus = RegisterServiceCtrlHandler(StartupUtil::serviceName(), serviceCtrlHandler);
    if (g_service.hstatus) {
        g_service.status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_service.status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        g_service.status.dwWin32ExitCode = NO_ERROR;
        g_service.status.dwServiceSpecificExitCode = 0;
        g_service.status.dwCheckPoint = 0;
        g_service.status.dwWaitHint = 3000;

        reportServiceStatus(SERVICE_RUNNING);
    }
}

DWORD WINAPI serviceStart(void *arg)
{
    Q_UNUSED(arg);

    wchar_t name[2] = { 0 }; // ignored for SERVICE_WIN32_OWN_PROCESS
    SERVICE_TABLE_ENTRY table[] = { { name, serviceMain }, { nullptr, nullptr } };

    return !StartServiceCtrlDispatcher(table);
}

}

void ServiceWorker::run()
{
    DWORD id;
    const HANDLE hThr = CreateThread(nullptr, 8192, serviceStart, nullptr, 0, &id);
    CloseHandle(hThr);
}
