#ifndef ZONESSELECTOR_H
#define ZONESSELECTOR_H

#include <QPushButton>

class ZonesSelector : public QPushButton
{
    Q_OBJECT

public:
    explicit ZonesSelector(QWidget *parent = nullptr);

    bool isTristate() const { return m_isTristate; }
    void setIsTristate(bool isTristate) { m_isTristate = isTristate; }

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

    void resetZonesMenu();
    void clearZonesMenu();
    void createZonesMenu();
    void updateZonesMenu();
    void updateZonesMenuEnabled();

    void addZone(int zoneId);
    void removeZone(int zoneId);

    void onZoneClicked(bool checked);

private:
    bool m_isTristate = false;

    quint32 m_zones = 0;
    quint32 m_uncheckedZones = 0;

    QMenu *m_menuZones = nullptr;
};

#endif // ZONESSELECTOR_H
