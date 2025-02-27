#ifndef GROUPSCONTROLLER_H
#define GROUPSCONTROLLER_H

#include <form/basecontroller.h>

class Group;

class GroupsController : public BaseController
{
    Q_OBJECT

public:
    explicit GroupsController(QObject *parent = nullptr);

public slots:
    bool addOrUpdateGroup(Group &group);
    void deleteGroup(int groupId);
    bool updateGroupName(int groupId, const QString &groupName);
};

#endif // GROUPSCONTROLLER_H
