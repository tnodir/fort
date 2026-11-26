#ifndef SERVICEWORKER_H
#define SERVICEWORKER_H

#include <QObject>

class ServiceManagerIface;

class ServiceWorker
{
public:
    // Connects to the Service Control Manager and reports the pending start
    static void run(ServiceManagerIface *manager);

    // Reports the running service: the Service Control Manager's auto-start waits for it,
    // so the service starts the driver only after it
    static void setStarted();
};

#endif // SERVICEWORKER_H
