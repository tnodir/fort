#include "servicemonitor.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

namespace {

inline void startServiceMonitor(const ServiceMonitor &monitor, PSERVICE_NOTIFYW notifyBuffer)
{
    const auto serviceHandle = SC_HANDLE(monitor.serviceHandle());

    constexpr DWORD notifyMask = SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_DELETE_PENDING;

    NotifyServiceStatusChangeW(serviceHandle, notifyMask, notifyBuffer);
}

void CALLBACK serviceNotifyCallback(PVOID param)
{
    auto notifyBuffer = PSERVICE_NOTIFYW(param);
    auto monitor = static_cast<ServiceMonitor *>(notifyBuffer->pContext);

    DWORD notificationTriggered = 0;
    DWORD processId = 0;

    if (notifyBuffer->dwNotificationStatus == ERROR_SUCCESS) {
        notificationTriggered = notifyBuffer->dwNotificationTriggered;
        processId = notifyBuffer->ServiceStatus.dwProcessId;
    }

    QMetaObject::invokeMethod(
            monitor, [=] { monitor->onServiceNotify(notificationTriggered, processId); },
            Qt::QueuedConnection);
}

}

ServiceMonitor::ServiceMonitor(const QString &serviceName, QObject *parent) :
    QObject(parent), m_serviceName(serviceName)
{
}

ServiceMonitor::~ServiceMonitor()
{
    stopMonitor();
}

void ServiceMonitor::startMonitor(void *managerHandle)
{
    m_serviceHandle = OpenServiceW(SC_HANDLE(managerHandle), PCWSTR(serviceName().utf16()),
            SERVICE_QUERY_STATUS | SERVICE_INTERROGATE);
    if (!m_serviceHandle)
        return;

    m_buffer.resize(sizeof(SERVICE_NOTIFYW));

    auto notifyBuffer = PSERVICE_NOTIFYW(m_buffer.data());
    RtlZeroMemory(notifyBuffer, sizeof(SERVICE_NOTIFYW));
    notifyBuffer->dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notifyBuffer->pfnNotifyCallback = &serviceNotifyCallback;
    notifyBuffer->pContext = this;

    startServiceMonitor(*this, notifyBuffer);
}

void ServiceMonitor::stopMonitor()
{
    if (!m_serviceHandle)
        return;

    CloseServiceHandle(SC_HANDLE(m_serviceHandle));
    m_serviceHandle = nullptr;
}

void ServiceMonitor::onServiceNotify(quint32 notificationTriggered, qint32 processId)
{
    if (!m_serviceHandle)
        return;

    auto notifyBuffer = PSERVICE_NOTIFYW(m_buffer.data());

    startServiceMonitor(*this, notifyBuffer);

    m_processId = processId;

    switch (notificationTriggered) {
    case SERVICE_NOTIFY_RUNNING: {
        m_state = ServiceRunning;
    } break;
    case SERVICE_NOTIFY_DELETE_PENDING: {
        m_state = ServiceDeleting;
    } break;
    }

    emit stateChanged();
}
