#include "groupsselector.h"

#include <conf/confgroupmanager.h>
#include <form/controls/pushbutton.h>
#include <fortglobal.h>
#include <model/grouplistmodel.h>
#include <util/conf/confutil.h>

using namespace Fort;

GroupsSelector::GroupsSelector(QWidget *parent) :
    MaskSelector(Fort::groupListModel(), WindowGroups, ":/icons/application_double.png", parent)
{
    setMaxCount(ConfUtil::groupMaxCount());

    connect(Fort::confGroupManager(), &ConfGroupManager::groupRemoved, this,
            &GroupsSelector::onIdRemoved);
}

void GroupsSelector::retranslateUi()
{
    MaskSelector::retranslateUi();

    menuButton()->setToolTip(
            tr("Select Groups.\n"
               "A program is enabled only if ANY of its exclusive groups is enabled,"
               " or, without exclusive groups, ANY of its groups is enabled."));
}

QString GroupsSelector::titleText() const
{
    return tr("Groups");
}

MaskSelectorItem GroupsSelector::itemAt(int row) const
{
    const auto &groupRow = Fort::groupListModel()->groupRowAt(row);

    const QString groupText =
            groupRow.exclusive ? groupRow.groupName + ' ' + tr("(exclusive)") : groupRow.groupName;

    return { groupRow.groupId, groupText };
}
