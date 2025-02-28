#include "zonesselector.h"

#include <QCheckBox>
#include <QMenu>
#include <QVBoxLayout>

#include <conf/confzonemanager.h>
#include <form/controls/controlutil.h>
#include <model/zonelistmodel.h>
#include <util/bitutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

namespace {

const char *const zoneIdPropertyName = "zoneId";

constexpr quint32 getZoneMask(int zoneId)
{
    return quint32(1) << (zoneId - 1);
}

constexpr void setZonesMask(quint32 &zones, int zoneId)
{
    zones |= getZoneMask(zoneId);
}

constexpr void clearZonesMask(quint32 &zones, int zoneId)
{
    zones &= ~getZoneMask(zoneId);
}

}

ZonesSelector::ZonesSelector(QWidget *parent) : PushButton(parent)
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
    return BitUtil::bitCount(m_zones);
}

int ZonesSelector::uncheckedZonesCount() const
{
    return BitUtil::bitCount(m_uncheckedZones);
}

void ZonesSelector::retranslateUi()
{
    retranslateZonesText();

    this->setToolTip(tr("Select Zones"));
}

void ZonesSelector::retranslateZonesText()
{
    QString countText;
    if (zonesCount() != 0) {
        countText += QString::number(zonesCount());
    }
    if (uncheckedZonesCount() != 0) {
        countText += '^' + QString::number(uncheckedZonesCount());
    }
    if (!countText.isEmpty()) {
        countText = " (" + countText + ')';
    }

    this->setText(tr("Zones") + countText);
}

void ZonesSelector::setupUi()
{
    setIcon(IconCache::icon(":/icons/ip_class.png"));

    setupZones();
}

void ZonesSelector::setupZones()
{
    m_menuLayout = new QVBoxLayout();

    m_menuZones = ControlUtil::createMenuByLayout(m_menuLayout, this);
    this->setMenu(m_menuZones);

    connect(m_menuZones, &QMenu::aboutToShow, this, &ZonesSelector::updateZonesMenu);

    setupZonesChanged();
}

void ZonesSelector::setupZonesChanged()
{
    auto confZoneManager = IoC<ConfZoneManager>();

    connect(confZoneManager, &ConfZoneManager::zoneRemoved, this, [&](int zoneId) {
        removeZone(zoneId);
        retranslateZonesText();
    });

    const auto refreshZonesMenu = [&] {
        clearZonesMenu();
        updateZonesMenuEnabled();
    };

    refreshZonesMenu();

    auto zoneListModel = IoC<ZoneListModel>();

    connect(zoneListModel, &ZoneListModel::modelReset, this, refreshZonesMenu);
    connect(zoneListModel, &ZoneListModel::dataChanged, this, refreshZonesMenu);
}

void ZonesSelector::resetZonesMenu()
{
    clearZonesMenu();
    retranslateZonesText();
}

void ZonesSelector::clearZonesMenu()
{
    m_menuZones->close();

    ControlUtil::clearLayout(m_menuLayout);
}

void ZonesSelector::createZonesMenu()
{
    auto zoneListModel = IoC<ZoneListModel>();

    const int zoneCount = qMin(zoneListModel->rowCount(), maxZoneCount());
    for (int row = 0; row < zoneCount; ++row) {
        const auto &zoneRow = zoneListModel->zoneRowAt(row);

        auto cb = new QCheckBox(zoneRow.zoneName, m_menuZones);
        cb->setTristate(isTristate());
        cb->setProperty(zoneIdPropertyName, zoneRow.zoneId);

        connect(cb, &QCheckBox::clicked, this, &ZonesSelector::onZoneClicked);

        m_menuLayout->addWidget(cb);
    }
}

void ZonesSelector::updateZonesMenu()
{
    if (m_menuLayout->isEmpty()) {
        createZonesMenu();
    }

    int i = m_menuLayout->count();
    while (--i >= 0) {
        auto item = m_menuLayout->itemAt(i);
        auto cb = static_cast<QCheckBox *>(item->widget());

        const int zoneId = cb->property(zoneIdPropertyName).toInt();
        const quint32 zoneMask = getZoneMask(zoneId);
        const bool checked = (m_zones & zoneMask) != 0;
        const bool unchecked = (m_uncheckedZones & zoneMask) != 0;

        cb->setCheckState(checked
                        ? Qt::Checked
                        : (!unchecked && isTristate() ? Qt::PartiallyChecked : Qt::Unchecked));
    }
}

void ZonesSelector::updateZonesMenuEnabled()
{
    auto zoneListModel = IoC<ZoneListModel>();

    const bool isZoneExist = (zoneListModel->rowCount() != 0);

    this->setMenu(isZoneExist ? m_menuZones : nullptr);
}

void ZonesSelector::addZone(int zoneId)
{
    setZonesMask(m_zones, zoneId);
}

void ZonesSelector::removeZone(int zoneId)
{
    clearZonesMask(m_zones, zoneId);
}

void ZonesSelector::addUncheckedZone(int zoneId)
{
    setZonesMask(m_uncheckedZones, zoneId);
}

void ZonesSelector::removeUncheckedZone(int zoneId)
{
    clearZonesMask(m_uncheckedZones, zoneId);
}

void ZonesSelector::onZoneClicked(bool checked)
{
    auto cb = qobject_cast<QCheckBox *>(sender());
    const int zoneId = cb->property(zoneIdPropertyName).toInt();

    const bool isPartiallyChecked = (cb->checkState() == Qt::PartiallyChecked);

    if (isPartiallyChecked) {
        removeUncheckedZone(zoneId);
        checked = false;
    }

    if (checked) {
        addZone(zoneId);
    } else {
        removeZone(zoneId);

        if (isTristate() && !isPartiallyChecked) {
            addUncheckedZone(zoneId);
        }
    }

    emit zonesChanged();

    retranslateZonesText();
}
