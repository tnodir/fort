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
        if (notify->dwNotificationStatus == ERROR_SERVICE_MARKED_FOR_DELETE) {
            emit m->stateChanged(ServiceInfo::StateDeleted);
        }
        return;
    }

    m->setProcessId(notify->ServiceStatus.dwProcessId);

    const ServiceInfo::State state = (notify->ServiceStatus.dwCurrentState == SERVICE_STOPPED)
            ? ServiceInfo::StateInactive
            : ServiceInfo::StateActive;

    emit m->stateChanged(state);

    // NotifyServiceStatusChange() must not be called from the callback
    m->requestStartNotifier();
}

PSERVICE_NOTIFYW getNotifyBuffer(ServiceInfoMonitor *m)
{
    QByteArray &buffer = m->notifyBuffer();
    if (buffer.isNull()) {
        buffer = QByteArray(sizeof(SERVICE_NOTIFYW), '\0');
    }

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
    m_startNotifierRequested(false),
    m_processId(processId),
    m_name(name)
{
    openService(managerHandle);
}

ServiceInfoMonitor::~ServiceInfoMonitor()
{
    closeService();
}

void ServiceInfoMonitor::terminate()
{
    m_terminated = true;

    closeService();
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
        qCCritical(LC) << "Open service error:" << name() << GetLastError();
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
        return;
    }

    m_isReopening = true;

    closeService();
    openService();

    m_isReopening = false;
}

void ServiceInfoMonitor::startNotifier()
{
    if (m_terminated)
        return;

    m_startNotifierRequested = false;

    constexpr DWORD mask = SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING;

    PSERVICE_NOTIFYW notify = getNotifyBuffer(this);

    const DWORD res = NotifyServiceStatusChangeW(serviceHandle(), mask, notify);

    if (res != ERROR_SUCCESS) {
        if (res == ERROR_SERVICE_NOTIFY_CLIENT_LAGGING) {
            qCDebug(LC) << "Notifier is lagging:" << name();
            reopenService();
        } else {
            qCCritical(LC) << "Start notifier error:" << name() << res;
        }
    }
}
