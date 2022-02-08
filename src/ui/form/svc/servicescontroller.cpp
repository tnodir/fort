#include "servicescontroller.h"

#include <conf/confmanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <model/servicelistmodel.h>
#include <util/ioc/ioccontainer.h>

ServicesController::ServicesController(QObject *parent) :
    QObject(parent), m_serviceListModel(new ServiceListModel(this))
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ServicesController::retranslateUi);
}

ConfManager *ServicesController::confManager() const
{
    return IoC<ConfManager>();
}

IniUser *ServicesController::iniUser() const
{
    return confManager()->iniUser();
}

TranslationManager *ServicesController::translationManager() const
{
    return IoC<TranslationManager>();
}

WindowManager *ServicesController::windowManager() const
{
    return IoC<WindowManager>();
}

void ServicesController::initialize()
{
    serviceListModel()->initialize();
}
