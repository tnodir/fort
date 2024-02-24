#ifndef SERVICEHANDLE_H
#define SERVICEHANDLE_H

#include <QObject>

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

    void createService(const wchar_t *serviceName, const wchar_t *serviceDisplay,
            const wchar_t *serviceGroup, const wchar_t *dependencies, const QString &command);
    bool deleteService();

    void setServiceDescription(const wchar_t *serviceDescription);
    void setupServiceRestartConfig();

private:
    void openService(const wchar_t *serviceName, quint32 managerAccess, quint32 serviceAccess);
    void closeService();

private:
    qintptr m_managerHandle = 0;
    qintptr m_serviceHandle = 0;
};

#endif // SERVICEHANDLE_H
