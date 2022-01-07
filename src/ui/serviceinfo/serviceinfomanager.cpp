#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

const QLoggingCategory LC("serviceInfo.serviceInfoManager");

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr, DWORD state = SERVICE_STATE_ALL)
{
    QVector<ServiceInfo> infoList;

    constexpr DWORD bufferMaxSize = 32 * 1024;
    ENUM_SERVICE_STATUS_PROCESSW buffer[bufferMaxSize / sizeof(ENUM_SERVICE_STATUS_PROCESSW)];
    DWORD bytesRemaining = 0;
    DWORD serviceCount = 0;
    DWORD resumePoint = 0;

    while (EnumServicesStatusExW(mngr, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, state, (LPBYTE) buffer,
                   sizeof(buffer), &bytesRemaining, &serviceCount, &resumePoint, nullptr)
            || GetLastError() == ERROR_MORE_DATA) {

        int infoIndex = infoList.size();
        infoList.resize(infoIndex + serviceCount);

        const ENUM_SERVICE_STATUS_PROCESSW *service = &buffer[0];

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

int ServiceInfoManager::groupIndexByName(const QString &name) const
{
    return m_serviceGroups.value(name, -1);
}

QVector<ServiceInfo> ServiceInfoManager::loadServiceInfoList()
{
    QVector<ServiceInfo> list;
    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (mngr) {
        list = getServiceInfoList(mngr);
        CloseServiceHandle(mngr);
    }
    return list;
}
