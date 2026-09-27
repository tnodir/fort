#include "groupsselector.h"

#include <QCheckBox>
#include <QMenu>
#include <QVBoxLayout>

#include <conf/confgroupmanager.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <model/grouplistmodel.h>
#include <util/bitutil.h>
#include <util/iconcache.h>

using namespace Fort;

namespace {

const char *const groupIdPropertyName = "groupId";

constexpr quint32 getGroupMask(int groupId)
{
    return quint32(1) << (groupId - 1);
}

constexpr void setGroupsMask(quint32 &groups, int groupId)
{
    groups |= getGroupMask(groupId);
}

constexpr void clearGroupsMask(quint32 &groups, int groupId)
{
    groups &= ~getGroupMask(groupId);
}

}

GroupsSelector::GroupsSelector(QWidget *parent) : PushButton(parent)
{
    setupUi();
}

void GroupsSelector::setGroups(quint32 groups)
{
    if (m_groups == groups)
        return;

    m_groups = groups;

    resetGroupsMenu();
}

int GroupsSelector::groupsCount() const
{
    return BitUtil::bitCount(m_groups);
}

void GroupsSelector::retranslateUi()
{
    retranslateGroupsText();

    this->setToolTip(tr("Select Groups.\n"
                        "A program is enabled only if ALL of its exclusive groups are enabled,"
                        " and at least one of its groups is enabled."));
}

void GroupsSelector::retranslateGroupsText()
{
    QString countText;
    if (groupsCount() != 0) {
        countText = " (" + QString::number(groupsCount()) + ')';
    }

    this->setText(tr("Groups") + countText);
}

void GroupsSelector::setupUi()
{
    setIcon(IconCache::icon(":/icons/application_double.png"));

    setupGroups();
}

void GroupsSelector::setupGroups()
{
    m_menuLayout = new QVBoxLayout();

    m_menuGroups = ControlUtil::createMenuByLayout(m_menuLayout, this);
    this->setMenu(m_menuGroups);

    connect(m_menuGroups, &QMenu::aboutToShow, this, &GroupsSelector::updateGroupsMenu);

    setupGroupsChanged();
}

void GroupsSelector::setupGroupsChanged()
{
    auto confGroupManager = Fort::confGroupManager();

    connect(confGroupManager, &ConfGroupManager::groupRemoved, this, [&](quint8 groupId) {
        if ((m_groups & getGroupMask(groupId)) == 0)
            return;

        removeGroup(groupId);

        emit groupsChanged();

        retranslateGroupsText();
    });

    const auto refreshGroupsMenu = [&] {
        clearGroupsMenu();
        updateGroupsMenuEnabled();
    };

    refreshGroupsMenu();

    auto groupListModel = Fort::groupListModel();

    connect(groupListModel, &GroupListModel::modelReset, this, refreshGroupsMenu);
    connect(groupListModel, &GroupListModel::dataChanged, this, refreshGroupsMenu);
}

void GroupsSelector::resetGroupsMenu()
{
    clearGroupsMenu();
    retranslateGroupsText();
}

void GroupsSelector::clearGroupsMenu()
{
    m_menuGroups->close();

    ControlUtil::clearLayout(m_menuLayout);
}

void GroupsSelector::createGroupsMenu()
{
    auto groupListModel = Fort::groupListModel();

    const int groupCount = qMin(groupListModel->rowCount(), maxGroupCount());
    for (int row = 0; row < groupCount; ++row) {
        const auto &groupRow = groupListModel->groupRowAt(row);

        const QString groupText = groupRow.exclusive ? groupRow.groupName + ' ' + tr("(exclusive)")
                                                     : groupRow.groupName;

        auto cb = new QCheckBox(groupText, m_menuGroups);
        cb->setProperty(groupIdPropertyName, groupRow.groupId);

        connect(cb, &QCheckBox::clicked, this, &GroupsSelector::onGroupClicked);

        m_menuLayout->addWidget(cb);
    }
}

void GroupsSelector::updateGroupsMenu()
{
    if (m_menuLayout->isEmpty()) {
        createGroupsMenu();
    }

    int i = m_menuLayout->count();
    while (--i >= 0) {
        auto item = m_menuLayout->itemAt(i);
        auto cb = static_cast<QCheckBox *>(item->widget());

        const int groupId = cb->property(groupIdPropertyName).toInt();
        const bool checked = (m_groups & getGroupMask(groupId)) != 0;

        cb->setChecked(checked);
    }
}

void GroupsSelector::updateGroupsMenuEnabled()
{
    auto groupListModel = Fort::groupListModel();

    const bool isGroupExist = (groupListModel->rowCount() != 0);

    this->setMenu(isGroupExist ? m_menuGroups : nullptr);
}

void GroupsSelector::addGroup(int groupId)
{
    setGroupsMask(m_groups, groupId);
}

void GroupsSelector::removeGroup(int groupId)
{
    clearGroupsMask(m_groups, groupId);
}

void GroupsSelector::onGroupClicked(bool checked)
{
    auto cb = qobject_cast<QCheckBox *>(sender());
    const int groupId = cb->property(groupIdPropertyName).toInt();

    if (checked) {
        addGroup(groupId);
    } else {
        removeGroup(groupId);
    }

    emit groupsChanged();

    retranslateGroupsText();
}
