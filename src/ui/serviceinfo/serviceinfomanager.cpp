#include "serviceinfomanager.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr)
{
    QVector<ServiceInfo> infoList;

    constexpr DWORD bufferSize = 32 * 1024;
    BYTE buffer[bufferSize];
    DWORD bytesRemaining = 0;
    DWORD serviceCount = 0;
    DWORD resumePoint = 0;

    while (EnumServicesStatusExW(mngr, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
            buffer, bufferSize, &bytesRemaining, &serviceCount, &resumePoint, nullptr)) {

        int infoIndex = infoList.size();
        infoList.resize(infoIndex + serviceCount);

        const ENUM_SERVICE_STATUS_PROCESSW *service =
                reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESS *>(buffer);

        for (; serviceCount > 0; --serviceCount, ++service, ++infoIndex) {
            ServiceInfo &info = infoList[infoIndex];
            info.processId = service->ServiceStatusProcess.dwProcessId;
            info.serviceName = QString::fromUtf16((const char16_t *) service->lpServiceName);
            info.displayName = QString::fromUtf16((const char16_t *) service->lpDisplayName);
        }

        if (bytesRemaining == 0)
            break;
    }

    return infoList;
}

}

ServiceInfoManager::ServiceInfoManager(QObject *parent) : QObject(parent) { }

void ServiceInfoManager::setEnabled(bool v)
{
    if (m_enabled != v) {
        m_enabled = v;
        updateWorker();
        updateServices();
    }
}

const ServiceInfo &ServiceInfoManager::serviceInfoAt(int index) const
{
    if (index < 0 || index >= services().size()) {
        static const ServiceInfo g_nullServiceInfo;
        return g_nullServiceInfo;
    }
    return services()[index];
}

void ServiceInfoManager::updateWorker()
{
    if (enabled()) {
        startWorker();
    } else {
        stopWorker();
    }
}

void ServiceInfoManager::startWorker() { }

void ServiceInfoManager::stopWorker() { }

void ServiceInfoManager::updateServices()
{
    services().clear();

    if (enabled()) {
        loadServices();
    }

    emit servicesChanged();
}

void ServiceInfoManager::loadServices()
{
    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (mngr) {
        m_services = getServiceInfoList(mngr);
        CloseServiceHandle(mngr);
    }
}
