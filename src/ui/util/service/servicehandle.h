#ifndef SERVICEHANDLE_H
#define SERVICEHANDLE_H

#include <QObject>

struct CreateServiceArg
{
    const wchar_t *serviceName;
    const wchar_t *serviceDisplay;
    const wchar_t *serviceDescription;
    const wchar_t *serviceGroup;
    const wchar_t *dependencies;
    const wchar_t *command;
};

class ServiceHandle final
{
public:
    explicit ServiceHandle(
            const wchar_t *serviceName, quint32 managerAccess = 0, quint32 serviceAccess = 0);
    ~ServiceHandle();

    bool isManagerOpened() const { return m_managerHandle != 0; }
    bool isServiceOpened() const { return m_serviceHandle != 0; }

    bool queryIsRunning();

    bool startService();
    bool stopService();

    bool createService(const CreateServiceArg &csa);
    bool deleteService();

    bool setupServiceRestartConfig();

private:
    void openService(const wchar_t *serviceName, quint32 managerAccess, quint32 serviceAccess);
    void closeService();

private:
    qintptr m_managerHandle = 0;
    qintptr m_serviceHandle = 0;
};

#endif // SERVICEHANDLE_H
