#include "groupscontroller.h"

#include <conf/confgroupmanager.h>
#include <manager/windowmanager.h>
#include <model/grouplistmodel.h>
#include <util/ioc/ioccontainer.h>

namespace {

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(
            errorMessage, GroupsController::tr("Group Configuration Error"));
}

}

GroupsController::GroupsController(QObject *parent) : BaseController(parent) { }

GroupListModel *GroupsController::groupListModel() const
{
    return IoC<GroupListModel>();
}

bool GroupsController::addOrUpdateGroup(Group &group)
{
    if (!confGroupManager()->addOrUpdateGroup(group)) {
        showErrorMessage(tr("Cannot edit Group"));
        return false;
    }
    return true;
}

void GroupsController::deleteGroup(int groupId)
{
    if (!confGroupManager()->deleteGroup(groupId)) {
        showErrorMessage(tr("Cannot delete Group"));
    }
}

bool GroupsController::updateGroupName(int groupId, const QString &groupName)
{
    if (!confGroupManager()->updateGroupName(groupId, groupName)) {
        showErrorMessage(tr("Cannot update Group's name"));
        return false;
    }
    return true;
}
