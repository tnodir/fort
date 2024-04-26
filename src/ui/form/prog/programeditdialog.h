#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <model/applistmodel.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfAppManager;
class ConfManager;
class FirewallConf;
class FortManager;
class IniUser;
class LineEdit;
class PlainTextEdit;
class ProgramsController;
class SpinCombo;
class WindowManager;
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
    WindowManager *windowManager() const;
    AppListModel *appListModel() const;

    bool isEmpty() const { return m_appRow.appId == 0; }

    void initialize(const AppRow &appRow, const QVector<qint64> &appIdList = {});

protected:
    virtual void closeOnSave();

    void setAdvancedMode(bool on);

private:
    void initializePathNameRuleFields();
    void initializePathField(bool isSingleSelection, bool isPathEditable);
    void initializeNameField(bool isSingleSelection);
    void initializeRuleField(bool isSingleSelection);
    void initializeFocus();

    QPixmap appIcon(bool isSingleSelection) const;

    void setupController();

    void retranslateUi();
    void retranslatePathPlaceholderText();
    void retranslateScheduleAction();
    void retranslateScheduleType();
    void retranslateScheduleIn();
    virtual void retranslateWindowTitle();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupPathLayout();
    QLayout *setupNameLayout();
    void setupComboAppGroups();
    QLayout *setupActionsLayout();
    void setupActionsGroup();
    void setupAdvancedOptions();
    void setupChildOptions();
    void setupLogOptions();
    QLayout *setupZonesRuleLayout();
    QLayout *setupRuleLayout();
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

    QString getEditText() const;

    void selectRuleDialog();
    void editRuleDialog(int ruleId);

    void warnDangerousOption() const;

private:
    ProgramsController *m_ctrl = nullptr;

    QLabel *m_labelEditPath = nullptr;
    LineEdit *m_editPath = nullptr;
    PlainTextEdit *m_editWildcard = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelEditName = nullptr;
    LineEdit *m_editName = nullptr;
    QToolButton *m_btGetName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbKillProcess = nullptr;
    QButtonGroup *m_btgActions = nullptr;
    QCheckBox *m_cbUseGroupPerm = nullptr;
    QCheckBox *m_cbApplyChild = nullptr;
    QCheckBox *m_cbKillChild = nullptr;
    QCheckBox *m_cbParked = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbLogConn = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    LineEdit *m_editRuleName = nullptr;
    QToolButton *m_btSelectRule = nullptr;
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
