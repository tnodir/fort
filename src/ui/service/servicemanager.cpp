#include "servicemanager.h"

#include <fort_version.h>

#include "serviceworker.h"

void ServiceManager::runService()
{
    ServiceWorker::run();
}
