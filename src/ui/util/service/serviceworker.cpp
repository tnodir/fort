#include "serviceworker.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "servicemanageriface.h"

#include <util/osutil.h>

namespace {

ServiceManagerIface *g_manager = nullptr;

DWORD WINAPI serviceCtrlHandler(
        DWORD code, DWORD eventType, LPVOID /*eventData*/, LPVOID /*context*/)
{
    g_manager->processControl(code, eventType);

    return NO_ERROR;
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
}

DWORD WINAPI serviceStart(void *arg)
{
    Q_UNUSED(arg);

    OsUtil::setCurrentThreadName("ServiceWorker");

    wchar_t name[2] = { 0 }; // ignored for SERVICE_WIN32_OWN_PROCESS
    SERVICE_TABLE_ENTRY table[] = { { name, serviceMain }, { nullptr, nullptr } };

    return !StartServiceCtrlDispatcher(table);
}

}

void ServiceWorker::run(ServiceManagerIface *manager)
{
    Q_ASSERT(manager);
    g_manager = manager;

    DWORD id;
    const HANDLE hThr = CreateThread(nullptr, 8192, serviceStart, nullptr, 0, &id);
    CloseHandle(hThr);
}
