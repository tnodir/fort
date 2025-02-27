#ifndef GROUPSCONTROLLER_H
#define GROUPSCONTROLLER_H

#include <form/basecontroller.h>

class Group;
class GroupListModel;

class GroupsController : public BaseController
{
    Q_OBJECT

public:
    explicit GroupsController(QObject *parent = nullptr);

    GroupListModel *groupListModel() const;

public slots:
    bool addOrUpdateGroup(Group &group);
    void deleteGroup(int groupId);
    bool updateGroupName(int groupId, const QString &groupName);
};

#endif // GROUPSCONTROLLER_H
