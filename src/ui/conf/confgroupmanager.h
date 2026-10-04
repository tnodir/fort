#ifndef CONFGROUPMANAGER_H
#define CONFGROUPMANAGER_H

#include <QHash>
#include <QObject>

#include <util/classhelpers.h>
#include <util/conf/confgroupswalker.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"

class Group;

class ConfGroupManager : public ConfManagerBase, public ConfGroupsWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfGroupManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfGroupManager)

    void setUp() override;

    QString groupNameById(quint8 groupId);
    QStringList groupNamesByMask(quint32 groupsMask);

    virtual bool addOrUpdateGroup(Group &group);
    virtual bool deleteGroup(quint8 groupId);
    virtual bool updateGroupName(quint8 groupId, const QString &groupName);
    virtual bool updateGroupEnabled(quint8 groupId, bool enabled);

    bool loadGroupById(Group &group, int groupId) const;

    bool walkGroups(const std::function<walkGroupsCallback> &func) const override;

    void updateDriverGroups();
    void updateDriverGroupFlags();

signals:
    void groupAdded();
    void groupRemoved(quint8 groupId);
    void groupUpdated();

private:
    quint32 activeGroupsMask() const;

    void setupGroupNamesCache();
    void clearGroupNamesCache();

    static void fillGroup(Group &group, const SqliteStmt &stmt);

private:
    mutable QHash<quint8, QString> m_groupNamesCache;

    quint32 m_driverActiveMask = 0; // last written to the driver
};

#endif // CONFGROUPMANAGER_H
