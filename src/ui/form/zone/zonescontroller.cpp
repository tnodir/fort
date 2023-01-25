#include "zonescontroller.h"

#include <model/zonelistmodel.h>
#include <util/ioc/ioccontainer.h>

ZonesController::ZonesController(QObject *parent) : BaseController(parent) { }

ZoneListModel *ZonesController::zoneListModel() const
{
    return IoC<ZoneListModel>();
}
