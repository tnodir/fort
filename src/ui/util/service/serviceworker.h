#ifndef SERVICEWORKER_H
#define SERVICEWORKER_H

#include <QObject>

class ServiceManagerIface;

class ServiceWorker
{
public:
    static void run(ServiceManagerIface *manager);
};

#endif // SERVICEWORKER_H
