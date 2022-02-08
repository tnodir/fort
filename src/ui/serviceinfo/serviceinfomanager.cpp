#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/fileutil.h>
#include <util/regkey.h>

namespace {

const QLoggingCategory LC("serviceInfo.serviceInfoManager");

const char *const servicesSubKey = R"(SYSTEM\CurrentControlSet\Services)";

QString getServiceDll(const RegKey &svcReg, bool *expand = nullptr)
{
    QVariant dllPathVar = svcReg.value("ServiceDll", expand);
    if (dllPathVar.isNull()) {
        const RegKey paramsReg(svcReg, "Parameters");
        dllPathVar = paramsReg.value("ServiceDll", expand);
    }

    return dllPathVar.toString();
}

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr, DWORD state = SERVICE_STATE_ALL)
{
    QVector<ServiceInfo> infoList;

    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);

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
            if (!svcReg.contains("ServiceSidType")
                    || svcReg.value("SvcHostSplitDisable").toInt() != 0)
                continue;

            const auto imagePath = svcReg.value("ImagePath").toString();
            if (!imagePath.contains(R"(\system32\svchost.exe)", Qt::CaseInsensitive))
                continue;

            const QString dllPath = getServiceDll(svcReg);
            if (dllPath.isEmpty())
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

QVector<ServiceInfo> ServiceInfoManager::loadServiceInfoList(ServiceInfo::State state)
{
    QVector<ServiceInfo> list;
    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (mngr) {
        list = getServiceInfoList(mngr, state);
        CloseServiceHandle(mngr);
    }
    return list;
}

QString ServiceInfoManager::getSvcHostServiceDll(const QString &serviceName)
{
    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);
    const RegKey svcReg(servicesReg, serviceName);

    bool expand = false;
    const QString dllPath = getServiceDll(svcReg, &expand);

    return expand ? FileUtil::expandPath(dllPath) : dllPath;
}
