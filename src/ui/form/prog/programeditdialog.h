#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <conf/app.h>
#include <form/controls/formwindow.h>

class AppConnListModel;
class AppListModel;
class ConfAppManager;
class ConfManager;
class ConfRuleManager;
class FirewallConf;
class FortManager;
class IniUser;
class LineEdit;
class PlainTextEdit;
class ProgramEditController;
class SpinCombo;
class TableView;
class WindowManager;
class ZonesSelector;

class ProgramEditDialog : public FormWindow
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(
            ProgramEditController *ctrl, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    ProgramEditController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    ConfAppManager *confAppManager() const;
    ConfRuleManager *confRuleManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    AppConnListModel *appConnListModel() const { return m_appConnListModel; }

    bool isWildcard() const { return m_isWildcard; }
    bool isNew() const { return m_app.appId == 0; }

    void initialize(const App &app, const QVector<qint64> &appIdList = {});

protected:
    virtual void closeOnSave();

    void setAdvancedMode(bool on);

    quint16 currentRuleId() const { return m_currentRuleId; }
    void setCurrentRuleId(quint16 ruleId = 0) { m_currentRuleId = ruleId; }

private:
    enum class ApplyChildType : qint8 {
        Invalid = -1,
        ToChild = 0,
        ToSpecChild,
        FromParent,
    };

    void initializePathNameRuleFields(bool isSingleSelection = true);
    void initializePathField(bool isSingleSelection);
    void initializeNameField(bool isSingleSelection);
    void initializeRuleField(bool isSingleSelection);
    void initializeFocus();

    QIcon appIcon(bool isSingleSelection) const;

    void setupController();
    void setupRuleManager();

    void retranslateUi();
    void retranslatePathPlaceholderText();
    void retranslateComboApplyChild();
    void retranslateScheduleAction();
    void retranslateScheduleType();
    void retranslateScheduleIn();
    void retranslateTimedMenuActions();
    void retranslateTimedAction(QToolButton *bt);
    virtual void retranslateWindowTitle();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupPathLayout();
    QLayout *setupNameLayout();
    QLayout *setupApplyChildGroupLayout();
    void setupCbApplyChild();
    void setupComboAppGroups();
    QLayout *setupActionsLayout();
    void setupActionsGroup();
    QLayout *setupZonesRuleLayout();
    QLayout *setupRuleLayout();
    QLayout *setupScheduleLayout();
    void setupCbSchedule();
    void setupComboScheduleType();
    void setupQuickAction();
    void setupTimedMenu();
    void setupTimedMenuActions();
    void setupTimedAction();
    void setupTimedRemove();
    QToolButton *createTimedButton(const QString &iniKey);
    QLayout *setupButtonsLayout();
    void setupSwitchWildcard();
    void setupOptions();
    void setupChildOptionsLayout();
    void setupLogOptions();
    void setupConnections();
    void setupConnectionsMenuLayout();
    void closeConnectionsMenuLayout();
    void setupConnectionsModel();
    void setupTableConnList();
    void setupTableConnListHeader();

    void updateZonesRulesLayout();
    void updateApplyChild();
    void updateWildcard(bool isSingleSelection = true);
    void updateQuickAction();

    void selectQuickAction();
    void selectTimedMenuAction(int index);

    int timedActionMinutes(QToolButton *bt);
    void setTimedActionMinutes(QToolButton *bt, int minutes);

    void switchWildcardPaths();

    void fillEditName();

    void saveScheduleAction(App::ScheduleAction actionType, int minutes);

    bool save();
    bool saveApp(App &app);
    bool saveMulti(App &app);

    bool validateFields() const;
    void fillApp(App &app) const;
    void fillAppPath(App &app) const;
    void fillAppApplyChild(App &app) const;
    void fillAppEndTime(App &app) const;

    QString getEditText() const;

    void selectRuleDialog();
    void editRuleDialog(int ruleId);

    void warnDangerousOption() const;
    void warnRestartNeededOption() const;

private:
    bool m_isWildcard = false;

    App::ScheduleAction m_quickActionType = App::ScheduleBlock;

    quint16 m_currentRuleId = 0;

    ProgramEditController *m_ctrl = nullptr;
    AppConnListModel *m_appConnListModel = nullptr;

    QLabel *m_labelEditPath = nullptr;
    LineEdit *m_editPath = nullptr;
    PlainTextEdit *m_editWildcard = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelEditName = nullptr;
    LineEdit *m_editName = nullptr;
    QToolButton *m_btGetName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QCheckBox *m_cbApplyChild = nullptr;
    QComboBox *m_comboApplyChild = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbKillProcess = nullptr;
    QButtonGroup *m_btgActions = nullptr;

    QCheckBox *m_cbKillChild = nullptr;
    QCheckBox *m_cbParked = nullptr;
    QCheckBox *m_cbLogAllowedConn = nullptr;
    QCheckBox *m_cbLogBlockedConn = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    LineEdit *m_editRuleName = nullptr;
    QToolButton *m_btSelectRule = nullptr;

    QCheckBox *m_cbSchedule = nullptr;
    QComboBox *m_comboScheduleAction = nullptr;
    QComboBox *m_comboScheduleType = nullptr;
    SpinCombo *m_scScheduleIn = nullptr;
    QDateTimeEdit *m_dteScheduleAt = nullptr;

    QPushButton *m_btQuickAction = nullptr;
    QToolButton *m_btTimedAction = nullptr;
    QToolButton *m_btTimedRemove = nullptr;
    QToolButton *m_timedMenuOwner = nullptr; // transient
    QActionGroup *m_timedMenuActions = nullptr;
    QMenu *m_timedMenu = nullptr;

    QToolButton *m_btSwitchWildcard = nullptr;
    QPushButton *m_btOptions = nullptr;
    QPushButton *m_btConnections = nullptr;
    QBoxLayout *m_connectionsLayout = nullptr;
    TableView *m_connListView = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;
    QPushButton *m_btMenu = nullptr;

    App m_app;
    QVector<qint64> m_appIdList;
};

#endif // PROGRAMEDITDIALOG_H
