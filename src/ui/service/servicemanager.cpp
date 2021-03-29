#include "servicemanager.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

const char *const serviceName = "FortFirewallSvc";

}

ServiceManager::ServiceManager(QObject *parent) : QObject(parent) { }

bool ServiceManager::isServiceInstalled()
{
    bool res = false;
    const SC_HANDLE scHandle = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scHandle) {
        const SC_HANDLE serviceHandle = OpenServiceA(scHandle, serviceName, SERVICE_INTERROGATE);
        if (serviceHandle) {
            res = true;
            CloseServiceHandle(serviceHandle);
        }
        CloseServiceHandle(scHandle);
    }
    return res;
}
