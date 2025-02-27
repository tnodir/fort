#ifndef CONFGROUPSWALKER_H
#define CONFGROUPSWALKER_H

#include <QObject>

#include <functional>

class Group;

using walkGroupsCallback = bool(Group &group);

class ConfGroupsWalker
{
public:
    virtual bool walkGroups(const std::function<walkGroupsCallback> &func) const = 0;
};

#endif // CONFGROUPSWALKER_H
