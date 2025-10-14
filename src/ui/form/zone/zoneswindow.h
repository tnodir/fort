#ifndef ZONESWINDOW_H
#define ZONESWINDOW_H

#include <form/controls/formwindow.h>

class TableView;
class Zone;
class ZoneEditDialog;
class ZonesController;

class ZonesWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ZonesWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowZones; }
    QString windowOverlayIconPath() const override { return ":/icons/ip_class.png"; }

    ZonesController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupSaveAsText();
    void setupTaskRun();
    void setupTableZones();
    void setupTableZonesHeader();
    void setupTableZonesChanged();
    void setupZoneListModelChanged();

    void addNewZone();
    void editSelectedZone();

    void openZoneEditForm(const Zone &zone);

    void deleteZone(int row);
    void deleteSelectedZone();

    void downloadZones();

    int zoneListCurrentIndex() const;

private:
    ZonesController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddZone = nullptr;
    QAction *m_actEditZone = nullptr;
    QAction *m_actRemoveZone = nullptr;
    QToolButton *m_btSaveAsText = nullptr;
    QToolButton *m_btUpdateZones = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_zoneListView = nullptr;

    ZoneEditDialog *m_formZoneEdit = nullptr;
};

#endif // ZONESWINDOW_H
