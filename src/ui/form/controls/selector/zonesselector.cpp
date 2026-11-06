#include "zonesselector.h"

#include <conf/confzonemanager.h>
#include <form/controls/pushbutton.h>
#include <fortglobal.h>
#include <model/zonelistmodel.h>
#include <util/conf/confutil.h>

using namespace Fort;

ZonesSelector::ZonesSelector(QWidget *parent) :
    MaskSelector(Fort::zoneListModel(), WindowZones, ":/icons/ip_class.png", parent)
{
    setMaxCount(ConfUtil::zoneMaxCount());

    connect(Fort::confZoneManager(), &ConfZoneManager::zoneRemoved, this,
            &ZonesSelector::onIdRemoved);
}

void ZonesSelector::retranslateUi()
{
    MaskSelector::retranslateUi();

    menuButton()->setToolTip(tr("Select Zones"));
}

QString ZonesSelector::titleText() const
{
    return tr("Zones");
}

MaskSelectorItem ZonesSelector::itemAt(int row) const
{
    const auto &zoneRow = Fort::zoneListModel()->zoneRowAt(row);

    return { zoneRow.zoneId, zoneRow.zoneName };
}
