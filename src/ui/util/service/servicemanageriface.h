#ifndef SERVICEMANAGERIFACE_H
#define SERVICEMANAGERIFACE_H

#include <QObject>

class ServiceManagerIface
{
public:
    ServiceManagerIface() = default;
    virtual ~ServiceManagerIface() = default;

    void initialize(qintptr hstatus);

    virtual const wchar_t *serviceName() const = 0;

    virtual void processControl(quint32 code) = 0;

protected:
    static void reportStatus(quint32 code);
};

#endif // SERVICEMANAGERIFACE_H
