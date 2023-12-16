#ifndef ZONESSELECTOR_H
#define ZONESSELECTOR_H

#include <QPushButton>

class ZonesSelector : public QPushButton
{
    Q_OBJECT

public:
    explicit ZonesSelector(QWidget *parent = nullptr);

    quint32 zones() const { return m_zones; }
    void setZones(quint32 zones);

    int zonesCount() const;

    void retranslateUi();

signals:
    void zonesChanged();

private:
    void retranslateZonesText();

    void setupUi();
    void setupZones();

    void clearZonesMenu();
    void createZonesMenu();
    void updateZonesMenu();
    void updateZonesMenuEnabled();

    void addZone(int zoneId);
    void removeZone(int zoneId);

    void onZoneClicked(bool checked);

private:
    quint32 m_zones = 0;

    QMenu *m_menuZones = nullptr;
};

#endif // ZONESSELECTOR_H
