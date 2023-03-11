#ifndef SERVICEMANAGERIFACE_H
#define SERVICEMANAGERIFACE_H

#include <QObject>

#define FORT_SERVICE_CONTROL_UNINSTALL 128

class ServiceManagerIface
{
public:
    ServiceManagerIface() = default;
    virtual ~ServiceManagerIface() = default;

    void initialize(qintptr hstatus);

    virtual const wchar_t *serviceName() const = 0;

    virtual void processControl(quint32 code) = 0;

protected:
    virtual bool acceptStop() const { return true; }
    virtual bool acceptPauseContinue() const { return true; }

    void setupAcceptedControls();

    static void reportStatus(quint32 code = 0);
};

#endif // SERVICEMANAGERIFACE_H
