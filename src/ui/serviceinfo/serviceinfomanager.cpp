#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/regkey.h>

namespace {

const QLoggingCategory LC("serviceInfo.serviceInfoManager");

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr, DWORD state = SERVICE_STATE_ALL)
{
    QVector<ServiceInfo> infoList;

    const RegKey servicesReg(RegKey::HKLM, R"(SYSTEM\CurrentControlSet\Services)");

    constexpr DWORD bufferMaxSize = 32 * 1024;
    ENUM_SERVICE_STATUS_PROCESSW buffer[bufferMaxSize / sizeof(ENUM_SERVICE_STATUS_PROCESSW)];
    DWORD bytesRemaining = 0;
    DWORD serviceCount = 0;
    DWORD resumePoint = 0;

    while (EnumServicesStatusExW(mngr, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, state, (LPBYTE) buffer,
                   sizeof(buffer), &bytesRemaining, &serviceCount, &resumePoint, nullptr)
            || GetLastError() == ERROR_MORE_DATA) {

        const ENUM_SERVICE_STATUS_PROCESSW *service = &buffer[0];

        for (int infoIndex = infoList.size(); serviceCount > 0;
                --serviceCount, ++service, ++infoIndex) {
            const auto serviceName = QString::fromUtf16((const char16_t *) service->lpServiceName);

            const RegKey svcReg(servicesReg, serviceName);
            if (!svcReg.contains("ServiceSidType"))
                continue;

            const auto imagePath = svcReg.value("ImagePath").toString();
            if (!imagePath.contains(R"(\system32\svchost.exe)", Qt::CaseInsensitive))
                continue;

            ServiceInfo info;
            info.processId = service->ServiceStatusProcess.dwProcessId;
            info.serviceName = serviceName;
            info.displayName = QString::fromUtf16((const char16_t *) service->lpDisplayName);

            infoList.append(info);
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
