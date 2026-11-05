#include "zonesselector.h"

#include <conf/confzonemanager.h>
#include <fortglobal.h>
#include <model/zonelistmodel.h>
#include <util/conf/confutil.h>

using namespace Fort;

ZonesSelector::ZonesSelector(QWidget *parent) :
    MaskSelector(Fort::zoneListModel(), ":/icons/ip_class.png", parent)
{
    setMaxCount(ConfUtil::zoneMaxCount());

    setupZoneRemoved();
}

void ZonesSelector::retranslateUi()
{
    MaskSelector::retranslateUi();

    this->setToolTip(tr("Select Zones"));
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

void ZonesSelector::setupZoneRemoved()
{
    connect(Fort::confZoneManager(), &ConfZoneManager::zoneRemoved, this, [&](int zoneId) {
        removeCheckedId(zoneId);
        retranslateMaskText();
    });
}
