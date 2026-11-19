#ifndef SERVICEWORKER_H
#define SERVICEWORKER_H

#include <QObject>

class ServiceManagerIface;

class ServiceWorker
{
public:
    // Connects to the Service Control Manager and reports the pending start
    static void run(ServiceManagerIface *manager);

    // Reports the running service: the initialization is finished
    static void setStarted();
};

#endif // SERVICEWORKER_H
