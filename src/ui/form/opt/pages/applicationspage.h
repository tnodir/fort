#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(TabBar)
QT_FORWARD_DECLARE_CLASS(CheckSpinCombo)

class ApplicationsPage : public BasePage
{
    Q_OBJECT

public:
    explicit ApplicationsPage(OptionsController *ctrl = nullptr,
                              QWidget *parent = nullptr);

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
    void setupGroupFragmentPacket();
    void retranslateGroupLimits();
    void refreshGroup();
    void setupAppGroup();

    int appGroupsCount() const;
    AppGroup *appGroup() const { return m_appGroup; }
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
};

#endif // APPLICATIONSPAGE_H
