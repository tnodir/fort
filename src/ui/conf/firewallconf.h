#ifndef FIREWALLCONF_H
#define FIREWALLCONF_H

#include <QObject>
#include <QVariant>

#include "inioptions.h"

class AddressGroup;
class AppGroup;

class FirewallConf : public QObject
{
    Q_OBJECT

public:
    enum EditedFlag {
        NoneEdited = 0,
        OptEdited = 0x01,
        FlagsEdited = 0x02,
        IniEdited = 0x04,
        ServiceEdited = 0x08,
        TaskEdited = 0x10,
        AllEdited = (OptEdited | FlagsEdited | IniEdited | ServiceEdited | TaskEdited)
    };

    explicit FirewallConf(Settings *settings = nullptr, QObject *parent = nullptr);

    uint editedFlags() const { return m_editedFlags; }

    bool optEdited() const { return (m_editedFlags & OptEdited) != 0; }
    void setOptEdited() { m_editedFlags |= OptEdited; }

    bool flagsEdited() const { return (m_editedFlags & FlagsEdited) != 0; }
    void setFlagsEdited() { m_editedFlags |= FlagsEdited; }

    bool iniEdited() const { return (m_editedFlags & IniEdited) != 0; }
    void setIniEdited() { m_editedFlags |= IniEdited; }

    bool serviceEdited() const { return (m_editedFlags & ServiceEdited) != 0; }
    void setServiceEdited() { m_editedFlags |= ServiceEdited; }

    bool taskEdited() const { return (m_editedFlags & TaskEdited) != 0; }
    void setTaskEdited() { m_editedFlags |= TaskEdited; }

    bool anyEdited() const { return m_editedFlags != NoneEdited; }
    void resetEdited(bool v = false);

    bool provBoot() const { return m_provBoot; }
    void setProvBoot(bool provBoot);

    bool filterEnabled() const { return m_filterEnabled; }
    void setFilterEnabled(bool filterEnabled);

    bool filterLocals() const { return m_filterLocals; }
    void setFilterLocals(bool filterLocals);

    bool stopTraffic() const { return m_stopTraffic; }
    void setStopTraffic(bool stopTraffic);

    bool stopInetTraffic() const { return m_stopInetTraffic; }
    void setStopInetTraffic(bool stopInetTraffic);

    bool allowAllNew() const { return m_allowAllNew; }
    void setAllowAllNew(bool allowAllNew);

    bool logBlocked() const { return m_logBlocked; }
    void setLogBlocked(bool logBlocked);

    bool logStat() const { return m_logStat; }
    void setLogStat(bool logStat);

    bool logStatNoFilter() const { return m_logStatNoFilter; }
    void setLogStatNoFilter(bool logStatNoFilter);

    bool logAllowedIp() const { return m_logAllowedIp; }
    void setLogAllowedIp(bool logAllowedIp);

    bool logBlockedIp() const { return m_logBlockedIp; }
    void setLogBlockedIp(bool logBlockedIp);

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll);

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll);

    bool activePeriodEnabled() const { return m_activePeriodEnabled; }
    void setActivePeriodEnabled(bool activePeriodEnabled);

    QString activePeriodFrom() const { return m_activePeriodFrom; }
    void setActivePeriodFrom(const QString &activePeriodFrom);

    QString activePeriodTo() const { return m_activePeriodTo; }
    void setActivePeriodTo(const QString &activePeriodTo);

    quint32 appGroupBits() const;
    void setAppGroupBits(quint32 groupBits);

    bool appGroupEnabled(int groupIndex) const;

    AddressGroup *inetAddressGroup() const { return m_addressGroups.at(0); }

    const QList<AddressGroup *> &addressGroups() const { return m_addressGroups; }

    const AppGroup *appGroupAt(int index) const;
    QStringList appGroupNames() const;

    AppGroup *appGroupByName(const QString &name) const;

    const QList<AppGroup *> &appGroups() const { return m_appGroups; }

    const QVector<qint64> &removedAppGroupIdList() const { return m_removedAppGroupIdList; }
    void clearRemovedAppGroupIdList() const;

    QVariantMap servicesMap() const { return m_servicesMap; }
    void setServicesMap(const QVariantMap &servicesMap);

    IniOptions &ini() { return m_ini; }
    const IniOptions &ini() const { return m_ini; }

    void copyFlags(const FirewallConf &o);
    void copy(const FirewallConf &o);

    QVariant toVariant(bool onlyFlags = false) const;
    void fromVariant(const QVariant &v, bool onlyFlags = false);

    static QVariant editedFlagsToVariant(uint editedFlags);
    static uint editedFlagsFromVariant(const QVariant &v);

signals:
    void logStatChanged();
    void appGroupsChanged();

public slots:
    void addAppGroup(AppGroup *appGroup, int to = -1);
    AppGroup *addAppGroupByName(const QString &name);
    void moveAppGroup(int from, int to);
    void removeAppGroup(int from, int to);
    void addDefaultAppGroup();

    void setupDefaultAddressGroups();

    void prepareToSave();
    void afterSaved();

private:
    void setupAddressGroups();

    void loadAppGroupBits();
    void applyAppGroupBits();

    QVariant flagsToVariant() const;
    void flagsFromVariant(const QVariant &v);

    QVariant addressesToVariant() const;
    void addressesFromVariant(const QVariant &v);

    QVariant appGroupsToVariant() const;
    void appGroupsFromVariant(const QVariant &v);

    QVariant removedAppGroupIdListToVariant() const;
    void removedAppGroupIdListFromVariant(const QVariant &v);

private:
    uint m_editedFlags : 8;

    uint m_provBoot : 1;
    uint m_filterEnabled : 1;
    uint m_filterLocals : 1;
    uint m_stopTraffic : 1;
    uint m_stopInetTraffic : 1;
    uint m_allowAllNew : 1;

    uint m_logBlocked : 1;
    uint m_logStat : 1;
    uint m_logStatNoFilter : 1;

    uint m_logAllowedIp : 1;
    uint m_logBlockedIp : 1;

    uint m_appBlockAll : 1;
    uint m_appAllowAll : 1;

    uint m_activePeriodEnabled : 1;

    quint32 m_appGroupBits = 0;

    QString m_activePeriodFrom;
    QString m_activePeriodTo;

    QList<AddressGroup *> m_addressGroups;

    QList<AppGroup *> m_appGroups;
    mutable QVector<qint64> m_removedAppGroupIdList;

    QVariantMap m_servicesMap;

    IniOptions m_ini;
};

#endif // FIREWALLCONF_H
