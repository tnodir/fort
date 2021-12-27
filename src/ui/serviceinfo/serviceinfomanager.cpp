#include "serviceinfomanager.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "serviceinfomonitor.h"
#include "servicelistmonitor.h"

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

ServiceInfoManager::~ServiceInfoManager()
{
    clearServiceMonitors();
    stopServiceListMonitor();
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

    const auto services = getServiceInfoList(mngr);
    for (const auto &info : services) {
        startServiceMonitor(info.serviceName, info.processId, mngr);
    }

    startServiceListMonitor(mngr);
}

void ServiceInfoManager::clearServiceMonitors()
{
    const auto monitors = m_serviceMonitors.values();
    m_serviceMonitors.clear();

    for (ServiceInfoMonitor *m : monitors) {
        stopServiceMonitor(m);
    }
}

void ServiceInfoManager::startServiceMonitor(
        const QString &name, quint32 processId, void *managerHandle)
{
    auto m = new ServiceInfoMonitor(processId, name, managerHandle);

    connect(m, &ServiceInfoMonitor::stateChanged, this, &ServiceInfoManager::onServiceStateChanged,
            Qt::QueuedConnection);
    connect(
            m, &ServiceInfoMonitor::errorOccurred, this, [=] { stopServiceMonitor(m); },
            Qt::QueuedConnection);

    m_serviceMonitors.insert(name, m);
}

void ServiceInfoManager::stopServiceMonitor(ServiceInfoMonitor *m)
{
    m_serviceMonitors.remove(m->name());

    m->terminate();
    m->deleteLater();
}

void ServiceInfoManager::startServiceListMonitor(void *managerHandle)
{
    auto m = new ServiceListMonitor(managerHandle);

    connect(m, &ServiceListMonitor::serviceCreated, this, &ServiceInfoManager::onServiceCreated,
            Qt::QueuedConnection);
    connect(m, &ServiceListMonitor::errorOccurred, this,
            &ServiceInfoManager::stopServiceListMonitor, Qt::QueuedConnection);

    m_serviceListMonitor = m;
}

void ServiceInfoManager::stopServiceListMonitor()
{
    auto m = m_serviceListMonitor;
    if (!m)
        return;

    m_serviceListMonitor = nullptr;

    m->terminate();
    m->deleteLater();
}

void ServiceInfoManager::onServiceStateChanged(ServiceInfo::State state)
{
    const auto m = qobject_cast<ServiceInfoMonitor *>(sender());
    Q_ASSERT(m);

    if (state == ServiceInfo::StateDeleted) {
        stopServiceMonitor(m);
    }
}

void ServiceInfoManager::onServiceCreated(const QStringList &nameList)
{
    for (const auto &name : nameList) {
        startServiceMonitor(name);
    }
}
