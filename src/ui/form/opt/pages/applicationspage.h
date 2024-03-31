#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "optbasepage.h"

class AppGroup;
class AppsColumn;
class CheckSpinCombo;
class CheckTimePeriod;
class LabelDoubleSpin;
class LabelSpin;
class LineEdit;
class TabBar;
class TextArea2Splitter;

class ApplicationsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit ApplicationsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    AppGroup *appGroup() const;

    int appGroupIndex() const { return m_appGroupIndex; }
    void setAppGroupIndex(int v);

signals:
    void appGroupChanged();

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void retranslateGroupLimits();
    void retranslateAppsPlaceholderText();

    void setupUi();
    QLayout *setupHeader();
    void setupAddGroup();
    void setupRenameGroup();
    void setupTabBar();
    int addTab(const QString &text);
    QLayout *setupGroupHeader();
    void setupGroupEnabled();
    void setupGroupPeriod();
    void setupGroupPeriodEnabled();
    void setupGroupOptions();
    void setupGroupOptionFlags();
    void setupGroupLog();
    void setupGroupLimitIn();
    void setupGroupLimitOut();
    void setupGroupLimitLatency();
    void setupGroupLimitPacketLoss();
    void setupGroupLimitBufferSize();
    void setupKillApps();
    void setupBlockApps();
    void setupAllowApps();
    void setupSplitter();
    void setupSplitterButtons();
    void updateGroup();
    void setupAppGroup();

    const QList<AppGroup *> &appGroups() const;
    int appGroupsCount() const;
    AppGroup *appGroupByIndex(int index) const;
    void resetGroupName();

private:
    int m_appGroupIndex = -1;

    LineEdit *m_editGroupName = nullptr;
    QToolButton *m_btAddGroup = nullptr;
    QToolButton *m_btRenameGroup = nullptr;
    TabBar *m_tabBar = nullptr;
    QCheckBox *m_cbGroupEnabled = nullptr;
    CheckTimePeriod *m_ctpGroupPeriod = nullptr;
    QPushButton *m_btGroupOptions = nullptr;
    QCheckBox *m_cbApplyChild = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    CheckSpinCombo *m_cscLimitIn = nullptr;
    CheckSpinCombo *m_cscLimitOut = nullptr;
    LabelSpin *m_limitLatency = nullptr;
    LabelDoubleSpin *m_limitPacketLoss = nullptr;
    LabelSpin *m_limitBufferSizeIn = nullptr;
    LabelSpin *m_limitBufferSizeOut = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbLogConn = nullptr;
    AppsColumn *m_killApps = nullptr;
    AppsColumn *m_blockApps = nullptr;
    AppsColumn *m_allowApps = nullptr;
    QSplitter *m_killSplitter = nullptr;
    TextArea2Splitter *m_allowSplitter = nullptr;
    QToolButton *m_btSelectFile = nullptr;
};

#endif // APPLICATIONSPAGE_H
