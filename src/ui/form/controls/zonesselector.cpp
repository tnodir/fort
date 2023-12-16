#include "zonesselector.h"

#include <QMenu>

#include <conf/confmanager.h>
#include <driver/drivercommon.h>
#include <form/controls/controlutil.h>
#include <model/zonelistmodel.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

namespace {

constexpr quint32 getZoneMask(int zoneId)
{
    return quint32(1) << (zoneId - 1);
}

}

ZonesSelector::ZonesSelector(QWidget *parent) : QPushButton(parent)
{
    setupUi();
}

void ZonesSelector::setZones(quint32 zones)
{
    if (m_zones == zones)
        return;

    m_zones = zones;

    resetZonesMenu();
}

void ZonesSelector::setUncheckedZones(quint32 uncheckedZones)
{
    if (m_uncheckedZones == uncheckedZones)
        return;

    m_uncheckedZones = uncheckedZones;

    resetZonesMenu();
}

int ZonesSelector::zonesCount() const
{
    return DriverCommon::bitCount(m_zones);
}

int ZonesSelector::uncheckedZonesCount() const
{
    return DriverCommon::bitCount(m_uncheckedZones);
}

void ZonesSelector::retranslateUi()
{
    retranslateZonesText();

    this->setToolTip(tr("Select Zones"));
}

void ZonesSelector::retranslateZonesText()
{
    const auto countText = QString::number(zonesCount())
            + (isTristate() ? '/' + QString::number(uncheckedZonesCount()) : QString());

    this->setText(tr("Zones") + " (" + countText + ')');
}

void ZonesSelector::setupUi()
{
    setIcon(IconCache::icon(":/icons/ip_class.png"));

    setupZones();
}

void ZonesSelector::setupZones()
{
    m_menuZones = ControlUtil::createMenu(this);
    this->setMenu(m_menuZones);

    connect(m_menuZones, &QMenu::aboutToShow, this, &ZonesSelector::updateZonesMenu);

    auto confManager = IoC<ConfManager>();

    connect(confManager, &ConfManager::zoneRemoved, this, [&](int zoneId) {
        removeZone(zoneId);
        retranslateZonesText();
    });

    auto zoneListModel = IoC<ZoneListModel>();

    connect(zoneListModel, &ZoneListModel::modelChanged, this, [&] {
        clearZonesMenu();
        updateZonesMenuEnabled();
    });

    updateZonesMenuEnabled();
}

void ZonesSelector::resetZonesMenu()
{
    clearZonesMenu();
    retranslateZonesText();
}

void ZonesSelector::clearZonesMenu()
{
    m_menuZones->close();
    m_menuZones->clear();
}

void ZonesSelector::createZonesMenu()
{
    auto zoneListModel = IoC<ZoneListModel>();

    const int zoneCount = zoneListModel->rowCount();
    for (int row = 0; row < zoneCount; ++row) {
        const auto zoneRow = zoneListModel->zoneRowAt(row);

        auto action = new QAction(zoneRow.zoneName, m_menuZones);
        action->setCheckable(true);
        action->setData(zoneRow.zoneId);

        connect(action, &QAction::triggered, this, &ZonesSelector::onZoneClicked);

        m_menuZones->addAction(action);
    }
}

void ZonesSelector::updateZonesMenu()
{
    if (m_menuZones->isEmpty()) {
        createZonesMenu();
    }

    const auto actions = m_menuZones->actions();

    for (auto action : actions) {
        const int zoneId = action->data().toInt();
        const quint32 zoneMask = getZoneMask(zoneId);
        const bool checked = (m_zones & zoneMask) != 0;

        action->setChecked(checked);
    }
}

void ZonesSelector::updateZonesMenuEnabled()
{
    auto zoneListModel = IoC<ZoneListModel>();

    const bool isZoneExist = (zoneListModel->rowCount() != 0);

    this->setEnabled(isZoneExist);
}

void ZonesSelector::addZone(int zoneId)
{
    m_zones |= getZoneMask(zoneId);
}

void ZonesSelector::removeZone(int zoneId)
{
    m_zones &= ~getZoneMask(zoneId);
}

void ZonesSelector::onZoneClicked(bool checked)
{
    auto action = qobject_cast<QAction *>(sender());
    const int zoneId = action->data().toInt();

    if (checked) {
        addZone(zoneId);
    } else {
        removeZone(zoneId);
    }

    emit zonesChanged();

    retranslateZonesText();
}
