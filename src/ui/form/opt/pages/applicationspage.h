#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(AppsColumn)
QT_FORWARD_DECLARE_CLASS(CheckSpinCombo)
QT_FORWARD_DECLARE_CLASS(CheckTimePeriod)
QT_FORWARD_DECLARE_CLASS(TabBar)
QT_FORWARD_DECLARE_CLASS(TextArea2Splitter)

class ApplicationsPage : public BasePage
{
    Q_OBJECT

public:
    explicit ApplicationsPage(OptionsController *ctrl = nullptr,
                              QWidget *parent = nullptr);

    AppGroup *appGroup() const { return m_appGroup; }
    void setAppGroup(AppGroup *v);

signals:
    void appGroupChanged();

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupAddGroup();
    void setupRenameGroup();
    void setupBlockAllowAll();
    void setupTabBar();
    int addTab(const QString &text);
    QLayout *setupGroupHeader();
    void setupGroupOptions();
    void setupGroupLimitIn();
    void setupGroupLimitOut();
    static CheckSpinCombo *createGroupLimit();
    void setupGroupFragmentPacket();
    void setupGroupOptionsEnabled();
    void retranslateGroupLimits();
    void setupGroupEnabled();
    void setupGroupPeriod();
    void setupGroupPeriodEnabled();
    void setupBlockApps();
    void setupAllowApps();
    void retranslateAppsPlaceholderText();
    void setupSplitter();
    void refreshGroup();
    void setupAppGroup();

    int appGroupsCount() const;
    AppGroup *appGroupByIndex(int index) const;
    void resetGroupName();

    static QString formatSpeed(int kbytes);

private:
    AppGroup *m_appGroup = nullptr;

    QLineEdit *m_editGroupName = nullptr;
    QPushButton *m_btAddGroup = nullptr;
    QPushButton *m_btRenameGroup = nullptr;
    QCheckBox *m_cbBlockAll = nullptr;
    QCheckBox *m_cbAllowAll = nullptr;
    TabBar *m_tabBar = nullptr;
    QPushButton *m_btGroupOptions = nullptr;
    CheckSpinCombo *m_cscLimitIn = nullptr;
    CheckSpinCombo *m_cscLimitOut = nullptr;
    QCheckBox *m_cbFragmentPacket = nullptr;
    QCheckBox *m_cbGroupEnabled = nullptr;
    CheckTimePeriod *m_ctpGroupPeriod = nullptr;
    AppsColumn *m_blockApps = nullptr;
    AppsColumn *m_allowApps = nullptr;
    TextArea2Splitter *m_splitter = nullptr;
};

#endif // APPLICATIONSPAGE_H
