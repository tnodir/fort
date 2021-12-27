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
        const QString nameStr = QString::fromWCharArray(++name);
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

    if (notify->dwNotificationStatus != ERROR_SUCCESS)
        return;

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
    m_terminated = true;

    closeManager();
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
        return;
    }

    m_isReopening = true;

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
        if (res == ERROR_SERVICE_NOTIFY_CLIENT_LAGGING) {
            qCDebug(LC) << "Notifier is lagging";
            reopenManager();
        } else {
            qCCritical(LC) << "Start notifier error:" << res;
        }
    }
}
