#ifndef ZONESWINDOW_H
#define ZONESWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class ConfManager;
class FortManager;
class FortSettings;
class TableView;
class TaskManager;
class ZoneListModel;
class ZonesController;

class ZonesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ZonesWindow(FortManager *fortManager, QWidget *parent = nullptr);

protected slots:
    void onSaveWindowState();
    void onRestoreWindowState();

    void onRetranslateUi();

private:
    void setupController();

    void setupUi();
    void setupZoneEditForm();
    void setupComboSources();
    QLayout *setupHeader();
    void setupTableZones();
    void setupTableZonesHeader();
    void setupTableZonesChanged();
    void setupZoneListModelChanged();

    void updateZoneEditForm(bool editCurrentZone);
    bool saveZoneEditForm();

    void updateZone(int row, bool enabled);
    void deleteZone(int row);

    void updateSelectedZone(bool enabled);
    void deleteSelectedZone();

    int zoneListCurrentIndex() const;

    ZonesController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

private:
    bool m_formZoneIsNew = false;

    ZonesController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddZone = nullptr;
    QAction *m_actEditZone = nullptr;
    QAction *m_actRemoveZone = nullptr;
    QPushButton *m_btSaveAsText = nullptr;
    QLabel *m_labelZoneName = nullptr;
    QLineEdit *m_editZoneName = nullptr;
    QLabel *m_labelSource = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QCheckBox *m_cbCustomUrl = nullptr;
    QComboBox *m_comboSources = nullptr;
    QLabel *m_labelUrl = nullptr;
    QLineEdit *m_editUrl = nullptr;
    QLabel *m_labelFormData = nullptr;
    QLineEdit *m_editFormData = nullptr;
    QPushButton *m_btEditOk = nullptr;
    QPushButton *m_btEditCancel = nullptr;
    QDialog *m_formZoneEdit = nullptr;
    TableView *m_zoneListView = nullptr;
};

#endif // ZONESWINDOW_H
