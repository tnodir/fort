#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "optbasepage.h"

class AppGroup;
class AppsColumn;
class CheckSpinCombo;
class CheckTimePeriod;
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
    void setupBlockAllowAll();
    void setupTabBar();
    int addTab(const QString &text);
    QLayout *setupGroupHeader();
    void setupGroupEnabled();
    void setupGroupPeriod();
    void setupGroupPeriodEnabled();
    void setupGroupOptions();
    void setupGroupLimitIn();
    void setupGroupLimitOut();
    void setupGroupLogConn();
    void setupGroupFragmentPacket();
    void setupGroupOptionsEnabled();
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

    static CheckSpinCombo *createGroupLimit();

    static QString formatSpeed(int kbytes);

private:
    int m_appGroupIndex = -1;

    QLineEdit *m_editGroupName = nullptr;
    QPushButton *m_btAddGroup = nullptr;
    QPushButton *m_btRenameGroup = nullptr;
    QCheckBox *m_cbBlockAll = nullptr;
    QCheckBox *m_cbAllowAll = nullptr;
    TabBar *m_tabBar = nullptr;
    QCheckBox *m_cbGroupEnabled = nullptr;
    CheckTimePeriod *m_ctpGroupPeriod = nullptr;
    QPushButton *m_btGroupOptions = nullptr;
    CheckSpinCombo *m_cscLimitIn = nullptr;
    CheckSpinCombo *m_cscLimitOut = nullptr;
    QCheckBox *m_cbLogConn = nullptr;
    QCheckBox *m_cbFragmentPacket = nullptr;
    AppsColumn *m_blockApps = nullptr;
    AppsColumn *m_allowApps = nullptr;
    TextArea2Splitter *m_splitter = nullptr;
    QPushButton *m_btSelectFile = nullptr;
};

#endif // APPLICATIONSPAGE_H
