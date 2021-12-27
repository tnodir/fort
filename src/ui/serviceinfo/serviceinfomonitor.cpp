#include "serviceinfomonitor.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#define asServiceHandle(h) static_cast<SC_HANDLE>((h))
#define serviceHandle()    asServiceHandle(m_serviceHandle)

namespace {

const QLoggingCategory LC("serviceInfo.serviceInfoMonitor");

static void CALLBACK notifyCallback(PVOID parameter)
{
    PSERVICE_NOTIFYW notify = static_cast<PSERVICE_NOTIFYW>(parameter);
    ServiceInfoMonitor *m = static_cast<ServiceInfoMonitor *>(notify->pContext);

    if (notify->dwNotificationStatus != ERROR_SUCCESS) {
        qCDebug(LC) << "Callback error:" << m->name() << notify->dwNotificationStatus;
        m->requestReopenService();
        return;
    }

    if (notify->dwNotificationTriggered & SERVICE_NOTIFY_DELETE_PENDING) {
        emit m->stateChanged(ServiceInfo::StateDeleted);
        return;
    }

    const bool running = (notify->ServiceStatus.dwCurrentState == SERVICE_RUNNING);

    m->setRunning(running);
    if (running) {
        m->setProcessId(notify->ServiceStatus.dwProcessId);
    }

    const ServiceInfo::State state =
            running ? ServiceInfo::StateActive : ServiceInfo::StateInactive;

    emit m->stateChanged(state);

    // NotifyServiceStatusChange() must not be called from the callback
    m->requestStartNotifier();
}

PSERVICE_NOTIFYW getNotifyBuffer(ServiceInfoMonitor *m)
{
    auto &buffer = m->notifyBuffer();

    PSERVICE_NOTIFYW notify = reinterpret_cast<PSERVICE_NOTIFYW>(buffer.data());

    notify->dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notify->pfnNotifyCallback = notifyCallback;
    notify->pContext = m;

    return notify;
}

}

ServiceInfoMonitor::ServiceInfoMonitor(
        quint32 processId, const QString &name, void *managerHandle, QObject *parent) :
    QObject(parent),
    m_terminated(false),
    m_isReopening(false),
    m_reopenServiceRequested(false),
    m_startNotifierRequested(false),
    m_running(processId != 0),
    m_processId(processId),
    m_name(name),
    m_notifyBuffer(sizeof(SERVICE_NOTIFYW))
{
    openService(managerHandle);
}

ServiceInfoMonitor::~ServiceInfoMonitor()
{
    terminate();
}

void ServiceInfoMonitor::terminate()
{
    if (m_terminated)
        return;

    m_terminated = true;

    closeService();
}

void ServiceInfoMonitor::requestReopenService()
{
    if (m_reopenServiceRequested)
        return;

    m_reopenServiceRequested = true;

    QMetaObject::invokeMethod(this, &ServiceInfoMonitor::reopenService, Qt::QueuedConnection);
}

void ServiceInfoMonitor::requestStartNotifier()
{
    if (m_startNotifierRequested)
        return;

    m_startNotifierRequested = true;

    QMetaObject::invokeMethod(this, &ServiceInfoMonitor::startNotifier, Qt::QueuedConnection);
}

void ServiceInfoMonitor::openService(void *managerHandle)
{
    const SC_HANDLE mngr = managerHandle ? asServiceHandle(managerHandle)
                                         : OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (mngr) {
        m_serviceHandle = OpenServiceW(mngr, (LPCWSTR) name().utf16(), SERVICE_QUERY_STATUS);

        if (!managerHandle) {
            CloseServiceHandle(mngr);
        }
    }

    if (!m_serviceHandle) {
        const DWORD res = GetLastError();
        switch (res) {
        case ERROR_SERVICE_MARKED_FOR_DELETE: {
            emit stateChanged(ServiceInfo::StateDeleted);
        } break;
        default:
            qCCritical(LC) << "Open service error:" << name() << res;
            emit errorOccurred();
        }
        return;
    }

    startNotifier();
}

void ServiceInfoMonitor::closeService()
{
    if (m_serviceHandle) {
        CloseServiceHandle(serviceHandle());
        m_serviceHandle = nullptr;
    }
}

void ServiceInfoMonitor::reopenService()
{
    if (m_isReopening) {
        qCCritical(LC) << "Reopen service error:" << name();
        emit errorOccurred();
        return;
    }

    m_isReopening = true;

    m_reopenServiceRequested = false;

    closeService();
    openService();

    m_isReopening = false;
}

void ServiceInfoMonitor::startNotifier()
{
    if (m_terminated)
        return;

    m_startNotifierRequested = false;

    constexpr DWORD mask =
            SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_DELETE_PENDING;

    PSERVICE_NOTIFYW notify = getNotifyBuffer(this);

    const DWORD res = NotifyServiceStatusChangeW(serviceHandle(), mask, notify);

    if (res != ERROR_SUCCESS) {
        switch (res) {
        case ERROR_SERVICE_MARKED_FOR_DELETE: {
            emit stateChanged(ServiceInfo::StateDeleted);
        } break;
        case ERROR_SERVICE_NOTIFY_CLIENT_LAGGING: {
            qCDebug(LC) << "Notifier is lagging:" << name();
            reopenService();
        } break;
        default:
            qCCritical(LC) << "Start notifier error:" << name() << res;
            emit errorOccurred();
        }
    }
}
