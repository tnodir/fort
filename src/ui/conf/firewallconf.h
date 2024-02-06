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
        OptEdited = (1 << 0),
        FlagsEdited = (1 << 1),
        IniEdited = (1 << 2),
        TaskEdited = (1 << 3),
        AllEdited = (OptEdited | FlagsEdited | IniEdited | TaskEdited)
    };
    Q_DECLARE_FLAGS(EditedFlags, EditedFlag)

    explicit FirewallConf(Settings *settings = nullptr, QObject *parent = nullptr);

    EditedFlags editedFlags() const { return EditedFlags(m_editedFlags); }

    bool optEdited() const { return (m_editedFlags & OptEdited) != 0; }
    void setOptEdited() { m_editedFlags |= OptEdited; }

    bool flagsEdited() const { return (m_editedFlags & FlagsEdited) != 0; }
    void setFlagsEdited() { m_editedFlags |= FlagsEdited; }

    bool iniEdited() const { return (m_editedFlags & IniEdited) != 0; }
    void setIniEdited() { m_editedFlags |= IniEdited; }

    bool taskEdited() const { return (m_editedFlags & TaskEdited) != 0; }
    void setTaskEdited() { m_editedFlags |= TaskEdited; }

    bool anyEdited() const { return m_editedFlags != NoneEdited; }
    void resetEdited(bool v = false);

    bool bootFilter() const { return m_bootFilter; }
    void setBootFilter(bool bootFilter);

    bool filterEnabled() const { return m_filterEnabled; }
    void setFilterEnabled(bool filterEnabled);

    bool filterLocals() const { return m_filterLocals; }
    void setFilterLocals(bool filterLocals);

    bool blockTraffic() const { return m_blockTraffic; }
    void setBlockTraffic(bool blockTraffic);

    bool blockInetTraffic() const { return m_blockInetTraffic; }
    void setBlockInetTraffic(bool blockInetTraffic);

    bool allowAllNew() const { return m_allowAllNew; }
    void setAllowAllNew(bool allowAllNew);

    bool askToConnect() const { return m_askToConnect; }
    void setAskToConnect(bool askToConnect);

    bool logStat() const { return m_logStat; }
    void setLogStat(bool logStat);

    bool logStatNoFilter() const { return m_logStatNoFilter; }
    void setLogStatNoFilter(bool logStatNoFilter);

    bool logBlocked() const { return m_logBlocked; }
    void setLogBlocked(bool logBlocked);

    bool logAllowedIp() const { return m_logAllowedIp; }
    void setLogAllowedIp(bool logAllowedIp);

    bool logBlockedIp() const { return m_logBlockedIp; }
    void setLogBlockedIp(bool logBlockedIp);

    bool logAlertedBlockedIp() const { return m_logAlertedBlockedIp; }
    void setLogAlertedBlockedIp(bool logAlertedBlockedIp);

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll);

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll);

    int filterModeIndex() const;
    void setFilterModeIndex(int index);

    static QStringList filterModeNames();
    static QStringList filterModeIconPaths();

    bool activePeriodEnabled() const { return m_activePeriodEnabled; }
    void setActivePeriodEnabled(bool activePeriodEnabled);

    QString activePeriodFrom() const { return m_activePeriodFrom; }
    void setActivePeriodFrom(const QString &activePeriodFrom);

    QString activePeriodTo() const { return m_activePeriodTo; }
    void setActivePeriodTo(const QString &activePeriodTo);

    quint32 appGroupBits() const { return m_appGroupBits; }
    void setAppGroupBits(quint32 groupBits);

    void setupAppGroupBits(quint32 groupBits);

    bool appGroupEnabled(int groupIndex) const;

    AddressGroup *inetAddressGroup() const { return m_addressGroups.at(0); }

    const QList<AddressGroup *> &addressGroups() const { return m_addressGroups; }

    const AppGroup *appGroupAt(int index) const;
    QStringList appGroupNames() const;

    AppGroup *appGroupByName(const QString &name) const;

    const QList<AppGroup *> &appGroups() const { return m_appGroups; }

    bool checkDeprecatedAppGroups() const; // TODO: COMPAT: Remove after v4.1.0

    const QVector<qint64> &removedAppGroupIdList() const { return m_removedAppGroupIdList; }
    void clearRemovedAppGroupIdList() const;

    IniOptions &ini() { return m_ini; }
    const IniOptions &ini() const { return m_ini; }

    void copyFlags(const FirewallConf &o);
    void copy(const FirewallConf &o);

    QVariant toVariant(bool onlyEdited = false) const;
    void fromVariant(const QVariant &v, bool onlyEdited = false);

    static QVariant editedFlagsToVariant(uint editedFlags);
    static uint editedFlagsFromVariant(const QVariant &v);

signals:
    void appGroupsChanged();

public slots:
    void addAppGroup(AppGroup *appGroup);
    AppGroup *addAppGroupByName(const QString &name);
    void addDefaultAppGroup();
    void moveAppGroup(int from, int to);
    void removeAppGroup(int from, int to);

    void setupDefaultAddressGroups();

    void prepareToSave();
    void afterSaved();

private:
    void setupAddressGroups();

    void setAppGroupsEdited(int from, int to);

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
    uint m_editedFlags : 8 = AllEdited; // update all on load()!

    uint m_bootFilter : 1 = false;
    uint m_filterEnabled : 1 = true;
    uint m_filterLocals : 1 = false;
    uint m_blockTraffic : 1 = false;
    uint m_blockInetTraffic : 1 = false;
    uint m_allowAllNew : 1 = false;
    uint m_askToConnect : 1 = false;

    uint m_logStat : 1 = false;
    uint m_logStatNoFilter : 1 = false;
    uint m_logBlocked : 1 = false;

    uint m_logAllowedIp : 1 = false;
    uint m_logBlockedIp : 1 = false;
    uint m_logAlertedBlockedIp : 1 = false;

    uint m_appBlockAll : 1 = true;
    uint m_appAllowAll : 1 = false;

    uint m_activePeriodEnabled : 1 = false;

    quint32 m_appGroupBits = 0;

    QString m_activePeriodFrom;
    QString m_activePeriodTo;

    QList<AddressGroup *> m_addressGroups;

    QList<AppGroup *> m_appGroups;
    mutable QVector<qint64> m_removedAppGroupIdList;

    IniOptions m_ini;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FirewallConf::EditedFlags)

#endif // FIREWALLCONF_H
