#include "programscontroller.h"

#include <appinfo/appinfocache.h>
#include <model/applistmodel.h>
#include <util/ioc/ioccontainer.h>

ProgramsController::ProgramsController(QObject *parent) :
    BaseController(parent), m_appListModel(new AppListModel(this))
{
}

AppInfoCache *ProgramsController::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

void ProgramsController::initialize()
{
    appListModel()->initialize();
}
