#include "graphscontroller.h"

#include <conf/confmanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>

using namespace Fort;

GraphsController::GraphsController(QObject *parent) : BaseController(parent) { }

void GraphsController::saveGraph(IniUser &ini, int graphId)
{
    ini.addGraphWindowId(graphId);

    confManager()->saveIniUser(ini, /*onlyFlags=*/false);
}

void GraphsController::removeGraph(int graphId)
{
    Q_ASSERT(graphId != 0); // the main graph is always present

    windowManager()->deleteGraphWindow(graphId);

    auto &ini = Fort::iniUser();

    ini.removeGraphWindowId(graphId);

    confManager()->saveIniUser(ini, /*onlyFlags=*/false);
}
