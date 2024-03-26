#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/fileutil.h>
#include <util/regkey.h>
#include <util/service/servicelistmonitor.h>
#include <util/service/servicemonitor.h>

namespace {

const QLoggingCategory LC("manager.serviceInfo");

const char *const servicesSubKey = R"(SYSTEM\CurrentControlSet\Services)";
const char *const serviceImagePathKey = "ImagePath";
const char *const serviceImagePathOldKey = "_Fort_ImagePath";
const char *const serviceTypeKey = "Type";
const char *const serviceTypeOldKey = "_Fort_Type";
const char *const serviceTrackFlagsKey = "_FortTrackFlags";

QString getServiceDll(const RegKey &svcReg, bool *expand = nullptr)
{
    QVariant dllPathVar = svcReg.value("ServiceDll", expand);
    if (dllPathVar.isNull()) {
        const RegKey paramsReg(svcReg, "Parameters");
        dllPathVar = paramsReg.value("ServiceDll", expand);
    }

    return dllPathVar.toString();
}

QString resolveSvcHostServiceName(const RegKey &servicesReg, const QString &serviceName)
{
    const RegKey svcReg(servicesReg, serviceName);

    const quint32 serviceType = svcReg.value(serviceTypeKey).toUInt();

    // Check a per-user service
    if (serviceType == 224) {
        const int pos = serviceName.lastIndexOf('_');
        if (pos > 0) {
            return serviceName.left(pos);
        }
    }

    return serviceName;
}

bool checkIsSvcHostService(const RegKey &svcReg)
{
    const auto imagePath = svcReg.value(serviceImagePathKey).toString();
    if (!imagePath.contains(R"(\system32\svchost.exe)", Qt::CaseInsensitive))
        return false;

    if (!svcReg.contains("ServiceSidType") || svcReg.value("SvcHostSplitDisable").toInt() != 0)
        return false;

    const QString dllPath = getServiceDll(svcReg);
    if (dllPath.isEmpty())
        return false;

    return true;
}

void fillServiceInfoList(QVector<ServiceInfo> &infoList, const RegKey &servicesReg,
        const ENUM_SERVICE_STATUS_PROCESSW *service, DWORD serviceCount, bool displayName,
        int &runningCount)
{
    for (int infoIndex = infoList.size(); serviceCount > 0;
            --serviceCount, ++service, ++infoIndex) {

        auto serviceName = QString::fromUtf16((const char16_t *) service->lpServiceName);
        serviceName = resolveSvcHostServiceName(servicesReg, serviceName);

        const RegKey svcReg(servicesReg, serviceName);

        if (!checkIsSvcHostService(svcReg))
            continue;

        const quint16 trackFlags = svcReg.value(serviceTrackFlagsKey).toUInt();

        ServiceInfo info;
        info.isRunning = (service->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING);
        info.serviceType = ServiceInfo::Type(service->ServiceStatusProcess.dwServiceType);
        info.trackFlags = trackFlags;
        info.processId = service->ServiceStatusProcess.dwProcessId;
        info.serviceName = std::move(serviceName);

        if (displayName) {
            info.displayName = QString::fromUtf16((const char16_t *) service->lpDisplayName);
        }

        if (info.isRunning) {
            ++runningCount;
        }

        infoList.append(info);
    }
}

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr, DWORD serviceType = SERVICE_WIN32,
        DWORD state = SERVICE_STATE_ALL, bool displayName = true,
        int *runningServicesCount = nullptr)
{
    QVector<ServiceInfo> infoList;

    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);

    constexpr DWORD bufferMaxSize = 32 * 1024;
    ENUM_SERVICE_STATUS_PROCESSW buffer[bufferMaxSize / sizeof(ENUM_SERVICE_STATUS_PROCESSW)];
    DWORD bytesRemaining = 0;
    DWORD serviceCount = 0;
    DWORD resumePoint = 0;

    while (EnumServicesStatusExW(mngr, SC_ENUM_PROCESS_INFO, serviceType, state, (LPBYTE) buffer,
                   sizeof(buffer), &bytesRemaining, &serviceCount, &resumePoint, nullptr)
            || GetLastError() == ERROR_MORE_DATA) {

        const ENUM_SERVICE_STATUS_PROCESSW *service = &buffer[0];

        int runningCount = 0;
        fillServiceInfoList(
                infoList, servicesReg, service, serviceCount, displayName, runningCount);

        if (runningServicesCount) {
            *runningServicesCount += runningCount;
        }

        if (bytesRemaining == 0)
            break;
    }

    return infoList;
}

}

ServiceInfoManager::ServiceInfoManager(QObject *parent) : QObject(parent) { }

void ServiceInfoManager::setUp()
{
    setupServiceListMonitor();
}

QVector<ServiceInfo> ServiceInfoManager::loadServiceInfoList(ServiceInfo::Type serviceType,
        ServiceInfo::State state, bool displayName, int *runningServicesCount)
{
    QVector<ServiceInfo> list;
    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (mngr) {
        list = getServiceInfoList(mngr, serviceType, state, displayName, runningServicesCount);
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

void ServiceInfoManager::trackService(const QString &serviceName)
{
    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);
    RegKey svcReg(servicesReg, serviceName, RegKey::DefaultReadWrite);

    bool expand;
    const QString imagePath = svcReg.value(serviceImagePathKey, &expand).toString();
    svcReg.setValue(serviceImagePathOldKey, imagePath, expand);
    svcReg.setValue(serviceImagePathKey, imagePath + " -s " + serviceName, expand);

    const quint32 shareType = svcReg.value(serviceTypeKey).toUInt();
    svcReg.setValue(serviceTypeOldKey, shareType);
    svcReg.setValue(serviceTypeKey, 0x10); // Own process

    svcReg.setValue(serviceTrackFlagsKey, ServiceInfo::RegImagePath | ServiceInfo::RegType);
}

void ServiceInfoManager::revertService(const QString &serviceName)
{
    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);
    RegKey svcReg(servicesReg, serviceName, RegKey::DefaultReadWrite);

    bool expand;
    const QString imagePath = svcReg.value(serviceImagePathOldKey, &expand).toString();
    if (!imagePath.isEmpty()) {
        svcReg.setValue(serviceImagePathKey, imagePath, expand);
    }
    svcReg.removeValue(serviceImagePathOldKey);

    const quint32 shareType = svcReg.value(serviceTypeOldKey).toUInt();
    if (shareType != 0) {
        svcReg.setValue(serviceTypeKey, shareType);
    }
    svcReg.removeValue(serviceTypeOldKey);

    svcReg.removeValue(serviceTrackFlagsKey);
}

void ServiceInfoManager::monitorServices(const QVector<ServiceInfo> &serviceInfoList)
{
    for (const ServiceInfo &serviceInfo : serviceInfoList) {
        setupServiceMonitor(serviceInfo.serviceName);
    }
}

void ServiceInfoManager::setupServiceListMonitor()
{
    m_serviceListMonitor = new ServiceListMonitor(this);

    connect(m_serviceListMonitor, &ServiceListMonitor::servicesCreated, this,
            &ServiceInfoManager::onServicesCreated);

    m_serviceListMonitor->startMonitor();
}

void ServiceInfoManager::setupServiceMonitor(const QString &serviceName)
{
    if (isServiceMonitoring(serviceName))
        return;

    auto serviceMonitor = new ServiceMonitor(serviceName, m_serviceListMonitor);

    connect(serviceMonitor, &ServiceMonitor::stateChanged, this,
            [=, this] { onServiceStateChanged(serviceMonitor); });

    startServiceMonitor(serviceMonitor);
}

bool ServiceInfoManager::isServiceMonitoring(const QString &serviceName) const
{
    return m_serviceMonitors.contains(serviceName);
}

void ServiceInfoManager::startServiceMonitor(ServiceMonitor *serviceMonitor)
{
    m_serviceMonitors.insert(serviceMonitor->serviceName(), serviceMonitor);

    serviceMonitor->startMonitor(m_serviceListMonitor->managerHandle());
}

void ServiceInfoManager::stopServiceMonitor(ServiceMonitor *serviceMonitor)
{
    m_serviceMonitors.remove(serviceMonitor->serviceName());

    delete serviceMonitor;
}

void ServiceInfoManager::onServicesCreated(const QStringList &serviceNames)
{
    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);

    for (const QString &name : serviceNames) {
        const QString serviceName = resolveSvcHostServiceName(servicesReg, name);

        const RegKey svcReg(servicesReg, serviceName);

        if (!checkIsSvcHostService(svcReg))
            continue;

        setupServiceMonitor(serviceName);
    }
}

void ServiceInfoManager::onServiceStateChanged(ServiceMonitor *serviceMonitor)
{
    switch (serviceMonitor->state()) {
    case ServiceMonitor::ServiceStateUnknown: {
    } break;
    case ServiceMonitor::ServiceRunning: {
        onServiceStarted(serviceMonitor);
    } break;
    case ServiceMonitor::ServiceDeleting: {
        stopServiceMonitor(serviceMonitor);
    } break;
    }
}

void ServiceInfoManager::onServiceStarted(ServiceMonitor *serviceMonitor)
{
    constexpr int servicesCount = 1;

    QVector<ServiceInfo> services(servicesCount);

    ServiceInfo &info = services[0];
    info.isRunning = true;
    info.processId = serviceMonitor->processId();
    info.serviceName = serviceMonitor->serviceName();

    emit servicesStarted(services, servicesCount);
}
