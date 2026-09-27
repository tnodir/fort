#ifndef GROUPSSELECTOR_H
#define GROUPSSELECTOR_H

#include "pushbutton.h"

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

inline constexpr int DefaultMaxGroupCount = 32;

class GroupsSelector : public PushButton
{
    Q_OBJECT

public:
    explicit GroupsSelector(QWidget *parent = nullptr);

    int maxGroupCount() const { return m_maxGroupCount; }
    void setMaxGroupCount(int maxGroupCount) { m_maxGroupCount = maxGroupCount; }

    quint32 groups() const { return m_groups; }
    void setGroups(quint32 groups);

    int groupsCount() const;

    void retranslateUi();

signals:
    void groupsChanged();

private:
    void retranslateGroupsText();

    void setupUi();
    void setupGroups();
    void setupGroupsChanged();

    void resetGroupsMenu();
    void clearGroupsMenu();
    void createGroupsMenu();
    void updateGroupsMenu();
    void updateGroupsMenuEnabled();

    void addGroup(int groupId);
    void removeGroup(int groupId);

    void onGroupClicked(bool checked);

private:
    int m_maxGroupCount = DefaultMaxGroupCount;

    quint32 m_groups = 0;

    QVBoxLayout *m_menuLayout = nullptr;
    QMenu *m_menuGroups = nullptr;
};

#endif // GROUPSSELECTOR_H
