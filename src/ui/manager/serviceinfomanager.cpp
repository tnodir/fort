#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/fileutil.h>
#include <util/regkey.h>
#include <util/service/servicehandle.h>
#include <util/service/servicelistmonitor.h>
#include <util/service/servicemonitor.h>

namespace {

const QLoggingCategory LC("manager.serviceInfo");

const char *const servicesSubKey = R"(SYSTEM\CurrentControlSet\Services)";
const char *const serviceImagePathKey = "ImagePath";
const char *const serviceImagePathOldKey = "_Fort_ImagePath";
const char *const serviceTypeKey = "Type";
const char *const serviceHostSplitDisableKey = "SvcHostSplitDisable";
const char *const serviceDllKey = "ServiceDll";
const char *const serviceParametersKey = "Parameters";
const char *const serviceTypeOldKey = "_Fort_Type";
const char *const serviceTrackFlagsKey = "_FortTrackFlags";

struct ServiceConfig
{
    bool expandImagePath = false;
    quint32 serviceType = 0;
    QString imagePath;
};

struct ServiceInfoListArgs
{
    bool displayName = true;
    QHash<quint32, int> processServicesCounts; // process id -> services count
    QVector<ServiceInfo> infoList;
};

QString getServiceDll(const RegKey &svcReg, bool *expand = nullptr)
{
    QVariant dllPathVar = svcReg.value(serviceDllKey, expand);
    if (dllPathVar.isNull()) {
        const RegKey paramsReg(svcReg, serviceParametersKey);
        dllPathVar = paramsReg.value(serviceDllKey, expand);
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

bool hasServiceDll(const RegKey &svcReg)
{
    if (!getServiceDll(svcReg).isEmpty())
        return true;

    // The non-elevated UI process may have no read access to the key (e.g. of "lmhosts")
    const RegKey paramsReg(svcReg, serviceParametersKey);

    return paramsReg.isAccessDenied();
}

bool checkIsSvcHostService(const RegKey &svcReg)
{
    const auto imagePath = svcReg.value(serviceImagePathKey).toString();
    if (!imagePath.contains(R"(\system32\svchost.exe)", Qt::CaseInsensitive))
        return false;

    if (!svcReg.contains("ServiceSidType"))
        return false;

    return hasServiceDll(svcReg);
}

quint16 getServiceTrackFlags(const RegKey &svcReg)
{
    return svcReg.value(serviceTrackFlagsKey).toUInt();
}

QString getServiceImagePathSuffix(const QString &serviceName)
{
    return " -s " + serviceName;
}

ServiceConfig getServiceConfig(const RegKey &svcReg)
{
    ServiceConfig conf;
    conf.imagePath = svcReg.value(serviceImagePathKey, &conf.expandImagePath).toString();
    conf.serviceType = svcReg.value(serviceTypeKey).toUInt();

    return conf;
}

// Change the SCM's database too to apply the changes on the service's restart
void setServiceConfig(RegKey &svcReg, const QString &serviceName, const ServiceConfig &conf)
{
    ServiceHandle svc((LPCWSTR) serviceName.utf16(), SC_MANAGER_CONNECT, SERVICE_CHANGE_CONFIG);
    if (svc.changeServiceConfig(conf.serviceType, (LPCWSTR) conf.imagePath.utf16()))
        return;

    qCWarning(LC) << "Change service config error:" << serviceName << GetLastError();

    // The changes will be applied on the system's restart
    svcReg.setValue(serviceImagePathKey, conf.imagePath, conf.expandImagePath);
    svcReg.setValue(serviceTypeKey, conf.serviceType);
}

void trackServiceImagePath(RegKey &svcReg, const QString &serviceName, ServiceConfig &conf)
{
    svcReg.setValue(serviceImagePathOldKey, conf.imagePath, conf.expandImagePath);
    conf.imagePath += getServiceImagePathSuffix(serviceName);
}

void trackServiceType(RegKey &svcReg, ServiceConfig &conf)
{
    svcReg.setValue(serviceTypeOldKey, conf.serviceType);
    conf.serviceType = ServiceInfo::TypeWin32OwnProcess;
}

void revertServiceImagePath(RegKey &svcReg, ServiceConfig &conf)
{
    bool expand = false;
    const QString imagePath = svcReg.value(serviceImagePathOldKey, &expand).toString();

    if (!imagePath.isEmpty()) {
        conf.imagePath = imagePath;
        conf.expandImagePath = expand;
    }

    svcReg.removeValue(serviceImagePathOldKey);
}

void revertServiceType(RegKey &svcReg, ServiceConfig &conf)
{
    const quint32 shareType = svcReg.value(serviceTypeOldKey).toUInt();

    if (shareType != 0) {
        conf.serviceType = shareType;
    }

    svcReg.removeValue(serviceTypeOldKey);
}

void fillServiceInfo(ServiceInfo &info, const RegKey &svcReg,
        const ENUM_SERVICE_STATUS_PROCESSW *service, bool displayName)
{
    const SERVICE_STATUS_PROCESS &status = service->ServiceStatusProcess;
    const quint16 trackFlags = getServiceTrackFlags(svcReg);

    info.hasProcess = (status.dwProcessId != 0);
    info.isHostSplitDisabled = svcReg.value(serviceHostSplitDisableKey).toInt() != 0;
    info.serviceType = ServiceInfo::Type(status.dwServiceType);
    info.trackFlags = trackFlags;
    info.processId = status.dwProcessId;

    if (displayName) {
        info.displayName = QString::fromUtf16((const char16_t *) service->lpDisplayName);
    }
}

void fillServiceInfoList(ServiceInfoListArgs &ila, const RegKey &servicesReg,
        const ENUM_SERVICE_STATUS_PROCESSW *service, DWORD serviceCount)
{
    for (; serviceCount > 0; --serviceCount, ++service) {
        const quint32 processId = service->ServiceStatusProcess.dwProcessId;

        // Count all services to check the shared processes
        if (processId != 0) {
            ++ila.processServicesCounts[processId];
        }

        const auto realServiceName = QString::fromUtf16((const char16_t *) service->lpServiceName);

        const auto serviceName = resolveSvcHostServiceName(servicesReg, realServiceName);
        const RegKey svcReg(servicesReg, serviceName);

        if (!checkIsSvcHostService(svcReg))
            continue;

        ServiceInfo info;
        info.serviceName = serviceName;
        info.realServiceName = realServiceName;

        fillServiceInfo(info, svcReg, service, ila.displayName);

        ila.infoList.append(info);
    }
}

bool checkIsServiceProcessShared(const ServiceInfoListArgs &ila, const ServiceInfo &info)
{
    if (!info.hasProcess)
        return false;

    return ila.processServicesCounts.value(info.processId) > 1;
}

void updateServiceInfoListProcesses(ServiceInfoListArgs &ila)
{
    for (ServiceInfo &info : ila.infoList) {
        info.isProcessShared = checkIsServiceProcessShared(ila, info);
    }
}

void getServiceInfoList(SC_HANDLE mngr, DWORD state, ServiceInfoListArgs &ila)
{
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

        fillServiceInfoList(ila, servicesReg, service, serviceCount);

        if (bytesRemaining == 0)
            break;
    }

    updateServiceInfoListProcesses(ila);
}

}

ServiceInfoManager::ServiceInfoManager(QObject *parent) : QObject(parent) { }

void ServiceInfoManager::setUp()
{
    setupServiceListMonitor();
}

QVector<ServiceInfo> ServiceInfoManager::loadServiceInfoList(
        ServiceInfo::State state, bool displayName)
{
    ServiceInfoListArgs ila = { .displayName = displayName };

    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (mngr) {
        getServiceInfoList(mngr, state, ila);
        CloseServiceHandle(mngr);
    }

    return ila.infoList;
}

QVector<ServiceInfo> ServiceInfoManager::ownProcessServices(
        const QVector<ServiceInfo> &serviceInfoList, int &processCount)
{
    QVector<ServiceInfo> list;

    for (const ServiceInfo &info : serviceInfoList) {
        // The changed service type may be not applied to the running service yet
        if (!info.isOwnProcess() || info.isProcessShared)
            continue;

        if (info.hasProcess) {
            ++processCount;
        }

        list.append(info);
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

    ServiceConfig conf = getServiceConfig(svcReg);

    trackServiceImagePath(svcReg, serviceName, conf);
    trackServiceType(svcReg, conf);

    setServiceConfig(svcReg, serviceName, conf);

    svcReg.setValue(serviceTrackFlagsKey, ServiceInfo::TrackImagePath | ServiceInfo::TrackType);
}

void ServiceInfoManager::revertService(const QString &serviceName)
{
    const RegKey servicesReg(RegKey::HKLM, servicesSubKey);
    RegKey svcReg(servicesReg, serviceName, RegKey::DefaultReadWrite);

    ServiceConfig conf = getServiceConfig(svcReg);

    revertServiceImagePath(svcReg, conf);
    revertServiceType(svcReg, conf);

    setServiceConfig(svcReg, serviceName, conf);

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
        const auto serviceName = resolveSvcHostServiceName(servicesReg, name);

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
    info.hasProcess = true;
    info.processId = serviceMonitor->processId();
    info.serviceName = serviceMonitor->serviceName();

    emit servicesStarted(services, servicesCount);
}
