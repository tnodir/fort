#ifndef ZONESWINDOW_H
#define ZONESWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QStackedLayout)

class ConfManager;
class IniOptions;
class IniUser;
class PlainTextEdit;
class TableView;
class TaskManager;
class WidgetWindowStateWatcher;
class WindowManager;
class Zone;
class ZoneListModel;
class ZoneSourceWrapper;
class ZonesController;

class ZonesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ZonesWindow(QWidget *parent = nullptr);

    quint32 windowCode() const override { return WindowZones; }

    ZonesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected slots:
    void onRetranslateUi();

private:
    void setupController();
    void setupStateWatcher();

    void setupUi();
    void setupZoneEditForm();
    QLayout *setupZoneEditNameLayout();
    void setupZoneEditSources();
    void setupZoneEditUrlFrame();
    QLayout *setupZoneEditUrlLayout();
    void setupZoneEditTextFrame();
    QLayout *setupZoneEditTextLayout();
    QLayout *setupZoneEditButtons();
    QLayout *setupHeader();
    void setupTableZones();
    void setupTableZonesHeader();
    void setupTableZonesChanged();
    void setupZoneListModelChanged();

    void updateZoneEditForm(bool isNew = false);
    void updateZoneEditFormBySource(const ZoneSourceWrapper &zoneSource);

    bool saveZoneEditForm();
    bool saveZoneEditFormValidate(const Zone &zone, const ZoneSourceWrapper &zoneSource);
    bool saveZoneEditFormNew(Zone &zone);
    bool saveZoneEditFormEdit(Zone &zone);

    void updateZone(int row, bool enabled);
    void deleteZone(int row);

    void updateSelectedZone(bool enabled);
    void deleteSelectedZone();

    int zoneListCurrentIndex() const;

private:
    bool m_formZoneIsNew = false;

    ZonesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddZone = nullptr;
    QAction *m_actEditZone = nullptr;
    QAction *m_actRemoveZone = nullptr;
    QPushButton *m_btSaveAsText = nullptr;
    QPushButton *m_btMenu = nullptr;
    QLabel *m_labelZoneName = nullptr;
    QLineEdit *m_editZoneName = nullptr;
    QLabel *m_labelSource = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QFrame *m_frameUrl = nullptr;
    QCheckBox *m_cbCustomUrl = nullptr;
    QComboBox *m_comboSources = nullptr;
    QLabel *m_labelUrl = nullptr;
    QLineEdit *m_editUrl = nullptr;
    QLabel *m_labelFormData = nullptr;
    QLineEdit *m_editFormData = nullptr;
    QFrame *m_frameText = nullptr;
    PlainTextEdit *m_editText = nullptr;
    QPushButton *m_btEditOk = nullptr;
    QPushButton *m_btEditCancel = nullptr;
    QDialog *m_formZoneEdit = nullptr;
    TableView *m_zoneListView = nullptr;
};

#endif // ZONESWINDOW_H
