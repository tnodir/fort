#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "serviceinfomonitor.h"

namespace {

const QLoggingCategory LC("serviceInfo.serviceInfoManager");

QVector<ServiceInfo> getServiceInfoList(SC_HANDLE mngr)
{
    QVector<ServiceInfo> infoList;

    constexpr DWORD bufferMaxSize = 32 * 1024;
    ENUM_SERVICE_STATUS_PROCESSW buffer[bufferMaxSize / sizeof(ENUM_SERVICE_STATUS_PROCESSW)];
    DWORD bytesRemaining = 0;
    DWORD serviceCount = 0;
    DWORD resumePoint = 0;

    while (EnumServicesStatusExW(mngr, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                   (LPBYTE) buffer, sizeof(buffer), &bytesRemaining, &serviceCount, &resumePoint,
                   nullptr)
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

ServiceInfoManager::~ServiceInfoManager()
{
    clearServiceMonitors();
}

void ServiceInfoManager::setMonitorEnabled(bool v)
{
    if (m_monitorEnabled != v) {
        m_monitorEnabled = v;

        setupServiceMonitors();
    }
}

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

void ServiceInfoManager::setupServiceMonitors()
{
    clearServiceMonitors();

    if (!monitorEnabled())
        return;

    const SC_HANDLE mngr =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!mngr) {
        qCCritical(LC) << "Open manager error:" << GetLastError();
        return;
    }

    const auto services = loadServiceInfoList();
    for (const auto &info : services) {
        const auto m = startServiceMonitor(info, mngr);

        m_serviceMonitors.insert(info.serviceName, m);
    }

    CloseServiceHandle(mngr);
}

void ServiceInfoManager::clearServiceMonitors()
{
    const auto moniros = m_serviceMonitors.values();
    for (ServiceInfoMonitor *m : moniros) {
        stopServiceMonitor(m);
    }
    m_serviceMonitors.clear();
}

ServiceInfoMonitor *ServiceInfoManager::startServiceMonitor(
        const ServiceInfo &info, void *managerHandle)
{
    const auto m = new ServiceInfoMonitor(info.processId, info.serviceName, managerHandle);

    connect(m, &ServiceInfoMonitor::stateChanged, this, &ServiceInfoManager::onServiceStateChanged);

    return m;
}

void ServiceInfoManager::stopServiceMonitor(ServiceInfoMonitor *m)
{
    m->terminate();
    m->deleteLater();
}

void ServiceInfoManager::onServiceStateChanged(ServiceInfo::State state)
{
    const auto m = qobject_cast<ServiceInfoMonitor *>(sender());
    Q_ASSERT(m);

    if (state == ServiceInfo::StateDeleted) {
        stopServiceMonitor(m);

        m_serviceMonitors.remove(m->name());
    }
}
