#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(TabBar)

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

    int appGroupsCount() const;
    AppGroup *appGroup(int tabIndex) const;
    void resetGroupName();

private:
    QLineEdit *m_editGroupName = nullptr;
    QPushButton *m_btAddGroup = nullptr;
    QPushButton *m_btRenameGroup = nullptr;
    QCheckBox *m_cbBlockAll = nullptr;
    QCheckBox *m_cbAllowAll = nullptr;
    TabBar *m_tabBar = nullptr;
};

#endif // APPLICATIONSPAGE_H
