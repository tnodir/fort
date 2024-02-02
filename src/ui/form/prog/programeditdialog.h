#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <model/applistmodel.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfAppManager;
class ConfManager;
class FirewallConf;
class FortManager;
class IniUser;
class PlainTextEdit;
class ProgramsController;
class SpinCombo;
class ZonesSelector;

class ProgramEditDialog : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(
            ProgramsController *ctrl, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    ProgramsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    ConfAppManager *confAppManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniUser *iniUser() const;
    AppListModel *appListModel() const;

    bool isEmpty() const { return m_appRow.appId == 0; }

    void initialize(const AppRow &appRow, const QVector<qint64> &appIdList = {});

protected:
    virtual void closeOnSave();

    void setAdvancedMode(bool on);

private:
    void initializePathNameFields();
    void initializePathField(bool isSingleSelection, bool isPathEditable);
    void initializeNameField(bool isSingleSelection, bool isPathEditable);

    void initializeFocus();

    void setupController();

    void retranslateUi();
    void retranslatePathPlaceholderText();
    void retranslateScheduleAction();
    void retranslateScheduleType();
    void retranslateScheduleIn();
    virtual void retranslateWindowTitle();

    void setupUi();
    QLayout *setupAppLayout();
    QLayout *setupAppPathLayout();
    QLayout *setupAppNameLayout();
    QLayout *setupNotesLayout();
    void setupComboAppGroups();
    QLayout *setupActionsLayout();
    void setupAdvancedOptions();
    QLayout *setupChildLayout();
    QLayout *setupLogLayout();
    QLayout *setupZonesLayout();
    QLayout *setupScheduleLayout();
    void setupCbSchedule();
    void setupComboScheduleType();
    QLayout *setupButtonsLayout();

    void fillEditName();

    bool save();
    bool saveApp(App &app);
    bool saveMulti(App &app);

    bool validateFields() const;
    void fillApp(App &app) const;
    void fillAppPath(App &app) const;
    void fillAppEndTime(App &app) const;

    bool isWildcard() const;

    void warnDangerousOption() const;

private:
    ProgramsController *m_ctrl = nullptr;

    QLabel *m_labelEditPath = nullptr;
    QLineEdit *m_editPath = nullptr;
    PlainTextEdit *m_editWildcard = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelEditName = nullptr;
    QLineEdit *m_editName = nullptr;
    QToolButton *m_btGetName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllowApp = nullptr;
    QRadioButton *m_rbBlockApp = nullptr;
    QRadioButton *m_rbKillProcess = nullptr;
    QCheckBox *m_cbUseGroupPerm = nullptr;
    QCheckBox *m_cbApplyChild = nullptr;
    QCheckBox *m_cbKillChild = nullptr;
    QCheckBox *m_cbParked = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbLogConn = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    QCheckBox *m_cbSchedule = nullptr;
    QComboBox *m_comboScheduleAction = nullptr;
    QComboBox *m_comboScheduleType = nullptr;
    SpinCombo *m_scScheduleIn = nullptr;
    QDateTimeEdit *m_dteScheduleAt = nullptr;
    QPushButton *m_btOptions = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    AppRow m_appRow;
    QVector<qint64> m_appIdList;
};

#endif // PROGRAMEDITDIALOG_H
