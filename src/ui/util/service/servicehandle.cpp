#include "servicehandle.h"

#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "servicemanageriface.h"

ServiceHandle::ServiceHandle(
        const wchar_t *serviceName, quint32 managerAccess, quint32 serviceAccess)
{
    openService(serviceName, managerAccess, serviceAccess);
}

ServiceHandle::~ServiceHandle()
{
    closeService();
}

bool ServiceHandle::queryIsRunning()
{
    SERVICE_STATUS status;
    if (QueryServiceStatus(SC_HANDLE(m_serviceHandle), &status)) {
        return (status.dwCurrentState == SERVICE_RUNNING);
    }
    return false;
}

bool ServiceHandle::startService()
{
    return StartServiceW(SC_HANDLE(m_serviceHandle), 0, nullptr);
}

bool ServiceHandle::stopService()
{
    int n = 3; /* count of attempts to stop the service */
    do {
        SERVICE_STATUS status;
        if (QueryServiceStatus(SC_HANDLE(m_serviceHandle), &status)
                && status.dwCurrentState == SERVICE_STOPPED)
            return true;

        const DWORD controlCode = (status.dwControlsAccepted & SERVICE_ACCEPT_STOP) != 0
                ? SERVICE_CONTROL_STOP
                : FORT_SERVICE_CONTROL_UNINSTALL;

        ControlService(SC_HANDLE(m_serviceHandle), controlCode, &status);

        QThread::msleep(n * 100);
    } while (--n > 0);

    return false;
}

bool ServiceHandle::createService(const CreateServiceArg &csa)
{
    m_serviceHandle = qintptr(CreateServiceW(SC_HANDLE(m_managerHandle), csa.serviceName,
            csa.serviceDisplay, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL, csa.command, csa.serviceGroup, nullptr, csa.dependencies, nullptr,
            nullptr));

    if (!isServiceOpened())
        return false;

    SERVICE_DESCRIPTION sd = { (LPWSTR) csa.serviceDescription };
    ChangeServiceConfig2(SC_HANDLE(m_serviceHandle), SERVICE_CONFIG_DESCRIPTION, &sd);

    return true;
}

bool ServiceHandle::deleteService()
{
    return DeleteService(SC_HANDLE(m_serviceHandle));
}

bool ServiceHandle::setupServiceRestartConfig()
{
    constexpr int actionsCount = 3;

    SC_ACTION actions[actionsCount];
    actions[0].Type = SC_ACTION_RESTART;
    actions[0].Delay = 150;
    actions[1].Type = SC_ACTION_NONE;
    actions[1].Delay = 0;
    actions[2].Type = SC_ACTION_NONE;
    actions[2].Delay = 0;

    SERVICE_FAILURE_ACTIONS sfa;
    sfa.dwResetPeriod = 0;
    sfa.lpCommand = nullptr;
    sfa.lpRebootMsg = nullptr;
    sfa.cActions = actionsCount;
    sfa.lpsaActions = actions;

    return ChangeServiceConfig2(SC_HANDLE(m_serviceHandle), SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);
}

void ServiceHandle::openService(
        const wchar_t *serviceName, quint32 managerAccess, quint32 serviceAccess)
{
    m_managerHandle = qintptr(OpenSCManagerW(nullptr, nullptr, managerAccess));
    if (!m_managerHandle)
        return;

    if (serviceAccess != 0) {
        m_serviceHandle =
                qintptr(OpenServiceW(SC_HANDLE(m_managerHandle), serviceName, serviceAccess));
    }
}

void ServiceHandle::closeService()
{
    if (m_serviceHandle) {
        CloseServiceHandle(SC_HANDLE(m_serviceHandle));
    }
    if (m_managerHandle) {
        CloseServiceHandle(SC_HANDLE(m_managerHandle));
    }
}
