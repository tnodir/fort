#ifndef PROGGENERALPAGE_H
#define PROGGENERALPAGE_H

#include <conf/app.h>

#include "progbasepage.h"

class LineEdit;
class PlainTextEdit;
class SpinCombo;

class ProgGeneralPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgGeneralPage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    bool validateFields() const override;
    void fillApp(App &app) const override;

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    enum class ApplyChildType : qint8 {
        Invalid = -1,
        ToChild = 0,
        ToSpecChild,
        FromParent,
    };

    enum ScheduleType : qint8 {
        ScheduleTimeIn = 0,
        ScheduleTimeAt,
    };

    void initializePathNameNotesFields(bool isSingleSelection = true);
    void initializePathField(bool isSingleSelection);
    void initializeNameField(bool isSingleSelection);
    void initializeNotesField(bool isSingleSelection);
    void initializeFocus();

    void retranslatePathPlaceholderText();
    void retranslateComboApplyChild();
    void retranslateScheduleAction();
    void retranslateScheduleType();
    void retranslateScheduleIn();
    void retranslateTimedMenuActions();
    void retranslateTimedAction(QToolButton *bt);

    void setupController();

    void setupUi();
    QLayout *setupFormLayout();
    QLayout *setupPathLayout();
    QLayout *setupNameLayout();
    QLayout *setupNotesLayout();
    QLayout *setupApplyChildGroupLayout();
    void setupCbApplyChild();
    void setupComboAppGroups();
    QLayout *setupActionsLayout();
    void setupActionsGroup();
    QLayout *setupScheduleLayout();
    void setupCbSchedule();
    void setupComboScheduleType();
    void setupQuickAction();
    void setupTimedMenu();
    void setupTimedMenuActions();
    void setupTimedAction();
    void setupTimedRemove();
    QToolButton *createTimedButton(const QString &iniKey);

    void updateWildcard();
    void updateApplyChild();
    void updateAppIcon();
    void updateQuickAction();

    void selectQuickAction();
    void selectTimedMenuAction(int index);

    int timedActionMinutes(QToolButton *bt);
    void setTimedActionMinutes(QToolButton *bt, int minutes);

    void fillEditName();

    QString getEditText() const;

    void onWildcardSwitched();

    void setIconPath(const QString &iconPath);

    QIcon appIcon(bool isSingleSelection) const;

    void saveScheduleAction(App::ScheduleAction actionType, int minutes);

    void fillAppPath(App &app) const;
    void fillAppApplyChild(App &app) const;
    void fillAppEndTime(App &app) const;

private:
    App::ScheduleAction m_quickActionType = App::ScheduleBlock;

    quint16 m_currentRuleId = 0;

    QLabel *m_labelEditPath = nullptr;
    LineEdit *m_editPath = nullptr;
    PlainTextEdit *m_editWildcard = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelEditName = nullptr;
    LineEdit *m_editName = nullptr;
    QToolButton *m_btGetName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    QString m_iconPath;
    PlainTextEdit *m_editNotes = nullptr;
    QToolButton *m_btSetIcon = nullptr;
    QToolButton *m_btDeleteIcon = nullptr;
    QCheckBox *m_cbApplyChild = nullptr;
    QComboBox *m_comboApplyChild = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbKillProcess = nullptr;
    QButtonGroup *m_btgActions = nullptr;

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
};

#endif // PROGGENERALPAGE_H
