#include "filtersimcontroller.h"

#include <conf/confmanager.h>
#include <conf/filtersimconn.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

FilterSimController::FilterSimController(QObject *parent) : BaseController(parent) { }

bool FilterSimController::simulateConn(FilterSimConn &simConn)
{
    if (!confManager()->simulateConn(simConn)) {
        windowManager()->showErrorBox(tr("Cannot simulate the connection"), tr("Filter Simulator"));
        return false;
    }
    return true;
}
