#ifndef ZONESSELECTOR_H
#define ZONESSELECTOR_H

#include "pushbutton.h"

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

constexpr int DefaultMaxZoneCount = 32;

class ZonesSelector : public PushButton
{
    Q_OBJECT

public:
    explicit ZonesSelector(QWidget *parent = nullptr);

    bool isTristate() const { return m_isTristate; }
    void setIsTristate(bool isTristate) { m_isTristate = isTristate; }

    int maxZoneCount() const { return m_maxZoneCount; }
    void setMaxZoneCount(int maxZoneCount) { m_maxZoneCount = maxZoneCount; }

    quint32 zones() const { return m_zones; }
    void setZones(quint32 zones);

    quint32 uncheckedZones() const { return m_uncheckedZones; }
    void setUncheckedZones(quint32 uncheckedZones);

    int zonesCount() const;
    int uncheckedZonesCount() const;

    void retranslateUi();

signals:
    void zonesChanged();
    void uncheckedZonesChanged();

private:
    void retranslateZonesText();

    void setupUi();
    void setupZones();
    void setupZonesChanged();

    void resetZonesMenu();
    void clearZonesMenu();
    void createZonesMenu();
    void updateZonesMenu();
    void updateZonesMenuEnabled();

    void addZone(int zoneId);
    void removeZone(int zoneId);

    void addUncheckedZone(int zoneId);
    void removeUncheckedZone(int zoneId);

    void onZoneClicked(bool checked);

private:
    bool m_isTristate = false;

    int m_maxZoneCount = DefaultMaxZoneCount;

    quint32 m_zones = 0;
    quint32 m_uncheckedZones = 0;

    QVBoxLayout *m_menuLayout = nullptr;
    QMenu *m_menuZones = nullptr;
};

#endif // ZONESSELECTOR_H
