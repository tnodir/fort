#ifndef ZONESWINDOW_H
#define ZONESWINDOW_H

#include <form/controls/formwindow.h>

class ConfManager;
class IniOptions;
class IniUser;
class TableView;
class TaskManager;
class WindowManager;
class ZoneEditDialog;
class ZoneListModel;
class ZonesController;

struct ZoneRow;

class ZonesWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ZonesWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowZones; }

    ZonesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

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

    void openZoneEditForm(const ZoneRow &zoneRow);

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
    QPushButton *m_btMenu = nullptr;
    TableView *m_zoneListView = nullptr;

    ZoneEditDialog *m_formZoneEdit = nullptr;
};

#endif // ZONESWINDOW_H
