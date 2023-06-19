#include "servicelistmonitor.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

inline void startManagerMonitor(const ServiceListMonitor &monitor, PSERVICE_NOTIFYW notifyBuffer)
{
    const auto managerHandle = SC_HANDLE(monitor.managerHandle());

    constexpr DWORD notifyMask = SERVICE_NOTIFY_CREATED;

    NotifyServiceStatusChangeW(managerHandle, notifyMask, notifyBuffer);
}

void parseServiceNames(
        PCWSTR names, QStringList &createdServiceNames, QStringList &deletedServiceNames)
{
    while (names[0] != L'\0') {
        const bool isCreated = (names[0] == L'/');
        if (isCreated) {
            ++names;
        }

        const auto name = QString::fromWCharArray(names);

        if (isCreated) {
            createdServiceNames.append(name);
        } else {
            deletedServiceNames.append(name);
        }

        names += name.size() + 1;
    }
}

void CALLBACK managerNotifyCallback(PVOID param)
{
    auto notifyBuffer = PSERVICE_NOTIFYW(param);
    auto monitor = static_cast<ServiceListMonitor *>(notifyBuffer->pContext);

    QStringList createdServiceNames;
    QStringList deletedServiceNames;

    if (notifyBuffer->dwNotificationStatus == ERROR_SUCCESS) {
        PWSTR names = notifyBuffer->pszServiceNames;
        if (names) {
            parseServiceNames(names, createdServiceNames, deletedServiceNames);

            LocalFree(names);
            notifyBuffer->pszServiceNames = nullptr;
        }
    }

    QMetaObject::invokeMethod(
            monitor, [=] { monitor->onManagerNotify(createdServiceNames); }, Qt::QueuedConnection);
}

}

ServiceListMonitor::ServiceListMonitor(QObject *parent) : QObject(parent) { }

ServiceListMonitor::~ServiceListMonitor()
{
    stopMonitor();
}

void ServiceListMonitor::startMonitor()
{
    m_managerHandle =
            OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!m_managerHandle)
        return;

    m_buffer.resize(sizeof(SERVICE_NOTIFYW));

    auto notifyBuffer = PSERVICE_NOTIFYW(m_buffer.data());
    RtlZeroMemory(notifyBuffer, sizeof(SERVICE_NOTIFYW));
    notifyBuffer->dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notifyBuffer->pfnNotifyCallback = &managerNotifyCallback;
    notifyBuffer->pContext = this;

    startManagerMonitor(*this, notifyBuffer);
}

void ServiceListMonitor::stopMonitor()
{
    if (!m_managerHandle)
        return;

    CloseServiceHandle(SC_HANDLE(m_managerHandle));
    m_managerHandle = nullptr;
}

void ServiceListMonitor::onManagerNotify(const QStringList &createdServiceNames)
{
    if (!m_managerHandle)
        return;

    auto notifyBuffer = PSERVICE_NOTIFYW(m_buffer.data());

    startManagerMonitor(*this, notifyBuffer);

    if (!createdServiceNames.isEmpty()) {
        emit servicesCreated(createdServiceNames);
    }
}
