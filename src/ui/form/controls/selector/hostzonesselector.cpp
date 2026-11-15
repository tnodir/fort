#include "hostzonesselector.h"

#include <form/controls/pushbutton.h>
#include <fortglobal.h>
#include <model/zonelistmodel.h>

HostZonesSelector::HostZonesSelector(QWidget *parent) : ZonesSelector(parent)
{
    refreshMenu();
}

void HostZonesSelector::retranslateUi()
{
    ZonesSelector::retranslateUi();

    menuButton()->setToolTip(tr("Select Hosts Zones"));
}

quint32 HostZonesSelector::listedMask() const
{
    return Fort::zoneListModel()->hostZonesMask();
}
