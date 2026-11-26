#include "serviceworker.h"

#include <QSemaphore>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "servicemanageriface.h"

#include <util/osutil.h>

namespace {

inline constexpr int startCheckPointPeriod = 1000;

ServiceManagerIface *g_manager = nullptr;

QSemaphore g_connected;
QSemaphore g_started;

DWORD WINAPI serviceCtrlHandler(
        DWORD code, DWORD eventType, LPVOID /*eventData*/, LPVOID /*context*/)
{
    g_manager->processControl(code, eventType);

    return NO_ERROR;
}

void waitStarted()
{
    // A pending service without the check points is considered hung
    while (!g_started.tryAcquire(1, startCheckPointPeriod)) {
        g_manager->reportStartPending();
    }
}

void WINAPI serviceMain(DWORD argc, wchar_t *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    const SERVICE_STATUS_HANDLE hstatus = RegisterServiceCtrlHandlerEx(
            g_manager->serviceName(), serviceCtrlHandler, /*context=*/nullptr);

    if (hstatus) {
        g_manager->initialize(qintptr(hstatus));
    }

    g_connected.release();

    if (hstatus) {
        waitStarted();
    }
}

DWORD WINAPI serviceStart(void *arg)
{
    Q_UNUSED(arg);

    OsUtil::setCurrentThreadName("ServiceWorker");

    wchar_t name[2] = { 0 }; // ignored for SERVICE_WIN32_OWN_PROCESS
    SERVICE_TABLE_ENTRY table[] = { { name, serviceMain }, { nullptr, nullptr } };

    if (StartServiceCtrlDispatcher(table))
        return 0;

    g_connected.release(); // not started by the Service Control Manager

    return 1;
}

}

void ServiceWorker::run(ServiceManagerIface *manager)
{
    Q_ASSERT(manager);
    g_manager = manager;

    DWORD id;
    const HANDLE hThr = CreateThread(nullptr, 0, serviceStart, nullptr, 0, &id);
    CloseHandle(hThr);

    // The status handle must be ready, when the initialization reports the running service
    g_connected.acquire();
}

void ServiceWorker::setStarted()
{
    g_manager->reportRunning();

    g_started.release();
}
