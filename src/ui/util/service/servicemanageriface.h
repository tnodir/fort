#ifndef SERVICEMANAGERIFACE_H
#define SERVICEMANAGERIFACE_H

#include <QObject>

#define FORT_SERVICE_CONTROL_UNINSTALL 128

class ServiceManagerIface
{
public:
    ServiceManagerIface() = default;
    virtual ~ServiceManagerIface() = default;

    virtual void initialize(qintptr hstatus);

    void registerDeviceNotification();
    void unregisterDeviceNotification();

    virtual const wchar_t *serviceName() const = 0;

    virtual void processControl(quint32 code, quint32 eventType) = 0;

protected:
    virtual bool acceptStop() const { return true; }
    virtual bool acceptPauseContinue() const { return true; }

    void setupAcceptedControls();

    static void reportStatus(quint32 code = 0);

    static bool isDeviceEvent(quint32 eventType);
};

#endif // SERVICEMANAGERIFACE_H
