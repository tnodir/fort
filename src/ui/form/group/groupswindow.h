#ifndef GROUPSWINDOW_H
#define GROUPSWINDOW_H

#include <form/controls/formwindow.h>

class GroupEditDialog;
class GroupListModel;
class GroupsController;
class TableView;
class WindowManager;

struct GroupRow;

class GroupsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit GroupsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowGroups; }
    QString windowOverlayIconPath() const override { return ":/icons/application_double.png"; }

    GroupsController *ctrl() const { return m_ctrl; }
    WindowManager *windowManager() const;
    GroupListModel *groupListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupTableGroups();
    void setupTableGroupsHeader();
    void setupTableGroupsChanged();
    void setupGroupListModelChanged();

    void addNewGroup();
    void editSelectedGroup();

    void openGroupEditForm(const GroupRow &groupRow);

    void deleteGroup(int row);
    void deleteSelectedGroup();

    int groupListCurrentIndex() const;

private:
    GroupsController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddGroup = nullptr;
    QAction *m_actEditGroup = nullptr;
    QAction *m_actRemoveGroup = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_groupListView = nullptr;

    GroupEditDialog *m_formGroupEdit = nullptr;
};

#endif // GROUPSWINDOW_H
