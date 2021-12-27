#include "servicelistmonitor.h"

#include <QLoggingCategory>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#define asManagerHandle(h) static_cast<SC_HANDLE>((h))
#define managerHandle()    asManagerHandle(m_managerHandle)

namespace {

const QLoggingCategory LC("serviceInfo.serviceListMonitor");

static QStringList parseServiceNames(LPWSTR pszServiceNames)
{
    QStringList list;
    PWSTR name = pszServiceNames;
    for (;;) {
        if (name[0] == '\0')
            break;
        Q_ASSERT(name[0] == '/');
        ++name; // Skip the leading '/' for created services
        const QString nameStr = QString::fromWCharArray(name);
        name += nameStr.length() + 1;
        list.append(nameStr);
    }
    LocalFree(pszServiceNames);
    return list;
}

static void CALLBACK notifyCallback(PVOID parameter)
{
    PSERVICE_NOTIFYW notify = static_cast<PSERVICE_NOTIFYW>(parameter);
    ServiceListMonitor *m = static_cast<ServiceListMonitor *>(notify->pContext);

    if (notify->dwNotificationStatus != ERROR_SUCCESS) {
        qCDebug(LC) << "Callback error:" << notify->dwNotificationStatus;
        m->requestReopenManager();
        return;
    }

    const QStringList nameList =
            notify->pszServiceNames ? parseServiceNames(notify->pszServiceNames) : QStringList();

    if (!nameList.isEmpty()) {
        emit m->serviceCreated(nameList);
    }

    // NotifyServiceStatusChange() must not be called from the callback
    m->requestStartNotifier();
}

PSERVICE_NOTIFYW getNotifyBuffer(ServiceListMonitor *m)
{
    auto &buffer = m->notifyBuffer();

    PSERVICE_NOTIFYW notify = reinterpret_cast<PSERVICE_NOTIFYW>(buffer.data());

    notify->dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    notify->pfnNotifyCallback = notifyCallback;
    notify->pContext = m;

    return notify;
}

}

ServiceListMonitor::ServiceListMonitor(void *managerHandle, QObject *parent) :
    QObject(parent),
    m_terminated(false),
    m_isReopening(false),
    m_reopenManagerRequested(false),
    m_startNotifierRequested(false),
    m_managerHandle(managerHandle),
    m_notifyBuffer(sizeof(SERVICE_NOTIFYW))
{
    openManager();
}

ServiceListMonitor::~ServiceListMonitor()
{
    terminate();
}

void ServiceListMonitor::terminate()
{
    if (m_terminated)
        return;

    m_terminated = true;

    closeManager();
}

void ServiceListMonitor::requestReopenManager()
{
    if (m_reopenManagerRequested)
        return;

    m_reopenManagerRequested = true;

    QMetaObject::invokeMethod(this, &ServiceListMonitor::reopenManager, Qt::QueuedConnection);
}

void ServiceListMonitor::requestStartNotifier()
{
    if (m_startNotifierRequested)
        return;

    m_startNotifierRequested = true;

    QMetaObject::invokeMethod(this, &ServiceListMonitor::startNotifier, Qt::QueuedConnection);
}

void ServiceListMonitor::openManager()
{
    if (!m_managerHandle) {
        m_managerHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);

        if (!m_managerHandle) {
            qCCritical(LC) << "Open manager error:" << GetLastError();
            emit errorOccurred();
            return;
        }
    }

    startNotifier();
}

void ServiceListMonitor::closeManager()
{
    if (m_managerHandle) {
        CloseServiceHandle(managerHandle());
        m_managerHandle = nullptr;
    }
}

void ServiceListMonitor::reopenManager()
{
    if (m_isReopening) {
        qCCritical(LC) << "Reopen manager error";
        emit errorOccurred();
        return;
    }

    m_isReopening = true;

    m_reopenManagerRequested = false;

    closeManager();
    openManager();

    m_isReopening = false;
}

void ServiceListMonitor::startNotifier()
{
    if (m_terminated)
        return;

    m_startNotifierRequested = false;

    constexpr DWORD mask = SERVICE_NOTIFY_CREATED;

    PSERVICE_NOTIFYW notify = getNotifyBuffer(this);

    const DWORD res = NotifyServiceStatusChangeW(managerHandle(), mask, notify);

    if (res != ERROR_SUCCESS) {
        switch (res) {
        case ERROR_SERVICE_NOTIFY_CLIENT_LAGGING: {
            qCDebug(LC) << "Notifier is lagging";
            reopenManager();
        } break;
        default:
            qCCritical(LC) << "Start notifier error:" << res;
            emit errorOccurred();
        }
    }
}
