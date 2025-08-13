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
        AutoLearnOff = (1 << 2),
        IniEdited = (1 << 3),
        TaskEdited = (1 << 4),
        AllEdited = (OptEdited | FlagsEdited | IniEdited | TaskEdited),
    };
    Q_ENUM(EditedFlag)
    Q_DECLARE_FLAGS(EditedFlags, EditedFlag)

    enum BlockTrafficType {
        BlockTrafficInvalid = -1,
        BlockTrafficNone = 0,
        BlockTrafficInet,
        BlockTrafficLan,
        BlockTrafficInetLan,
        BlockTrafficAll,
    };
    Q_ENUM(BlockTrafficType)

    enum FilterMode {
        ModeInvalid = -1,
        ModeAutoLearn = 0,
        ModeAskToConnect,
        ModeBlockAll,
        ModeAllowAll,
        ModeIgnore,
    };
    Q_ENUM(FilterMode)

    explicit FirewallConf(Settings *settings = nullptr, QObject *parent = nullptr);

    EditedFlags editedFlags() const { return EditedFlags(m_editedFlags); }

    bool optEdited() const { return (m_editedFlags & OptEdited) != 0; }
    void setOptEdited() { m_editedFlags |= OptEdited; }

    bool flagsEdited() const { return (m_editedFlags & FlagsEdited) != 0; }
    void setFlagsEdited() { m_editedFlags |= FlagsEdited; }

    bool autoLearnOff() const { return (m_editedFlags & AutoLearnOff) != 0; }
    void setAutoLearnOff() { m_editedFlags |= AutoLearnOff; }

    bool iniEdited() const { return (m_editedFlags & IniEdited) != 0; }
    void setIniEdited() { m_editedFlags |= IniEdited; }

    bool taskEdited() const { return (m_editedFlags & TaskEdited) != 0; }
    void setTaskEdited() { m_editedFlags |= TaskEdited; }

    bool anyEdited() const { return m_editedFlags != NoneEdited; }
    void resetEdited(EditedFlags v = NoneEdited) { m_editedFlags = v; }

    bool bootFilter() const { return m_bootFilter; }
    void setBootFilter(bool v) { m_bootFilter = v; }

    bool stealthMode() const { return m_stealthMode; }
    void setStealthMode(bool v) { m_stealthMode = v; }

    bool traceEvents() const { return m_traceEvents; }
    void setTraceEvents(bool v) { m_traceEvents = v; }

    bool filterEnabled() const { return m_filterEnabled; }
    void setFilterEnabled(bool v) { m_filterEnabled = v; }

    bool filterLocals() const { return m_filterLocals; }
    void setFilterLocals(bool v) { m_filterLocals = v; }

    bool filterLocalNet() const { return m_filterLocalNet; }
    void setFilterLocalNet(bool v) { m_filterLocalNet = v; }

    bool blockTraffic() const { return m_blockTraffic; }
    void setBlockTraffic(bool v) { m_blockTraffic = v; }

    bool blockLanTraffic() const { return m_blockLanTraffic; }
    void setBlockLanTraffic(bool v) { m_blockLanTraffic = v; }

    bool blockInetTraffic() const { return m_blockInetTraffic; }
    void setBlockInetTraffic(bool v) { m_blockInetTraffic = v; }

    bool allowAllNew() const { return m_allowAllNew; }
    void setAllowAllNew(bool v) { m_allowAllNew = v; }

    bool askToConnect() const { return m_askToConnect; }
    void setAskToConnect(bool v) { m_askToConnect = v; }

    bool groupBlocked() const { return m_groupBlocked; }
    void setGroupBlocked(bool v) { m_groupBlocked = v; }

    bool logStat() const { return m_logStat; }
    void setLogStat(bool v) { m_logStat = v; }

    bool logStatNoFilter() const { return m_logStatNoFilter; }
    void setLogStatNoFilter(bool v) { m_logStatNoFilter = v; }

    bool logApp() const { return m_logApp; }
    void setLogApp(bool v) { m_logApp = v; }

    bool logAllowedConn() const { return m_logAllowedConn; }
    void setLogAllowedConn(bool v) { m_logAllowedConn = v; }

    bool logBlockedConn() const { return m_logBlockedConn; }
    void setLogBlockedConn(bool v) { m_logBlockedConn = v; }

    bool logAlertedConn() const { return m_logAlertedConn; }
    void setLogAlertedConn(bool v) { m_logAlertedConn = v; }

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll) { m_appBlockAll = appBlockAll; }

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll) { m_appAllowAll = appAllowAll; }

    int blockTrafficIndex() const;
    void setBlockTrafficIndex(int index);

    FirewallConf::FilterMode filterMode() const;
    void setFilterMode(FirewallConf::FilterMode mode);

    static QStringList blockTrafficNames();
    static QStringList blockTrafficIconPaths();

    static QStringList filterModeNames();
    static QStringList filterModeIconPaths();

    bool activePeriodEnabled() const { return m_activePeriodEnabled; }
    void setActivePeriodEnabled(bool v) { m_activePeriodEnabled = v; }

    QString activePeriodFrom() const { return m_activePeriodFrom; }
    void setActivePeriodFrom(const QString &v) { m_activePeriodFrom = v; }

    QString activePeriodTo() const { return m_activePeriodTo; }
    void setActivePeriodTo(const QString &v) { m_activePeriodTo = v; }

    quint32 appGroupBits() const { return m_appGroupBits; }
    void setAppGroupBits(quint32 v) { m_appGroupBits = v; }

    quint32 activeGroupBits() const { return m_appGroupBits & m_groupActivePeriodBits; }

    void setupAppGroupBits(quint32 v);

    bool appGroupEnabled(int groupIndex) const;
    void setAppGroupEnabled(int groupIndex, bool v);

    AddressGroup *inetAddressGroup() const { return m_addressGroups.first(); }

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

    bool updateGroupPeriods(bool onlyFlags);

private:
    void setupAddressGroups();

    void setAppGroupsEdited(int from, int to);

    void loadGroupPeriodBits();

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
    uint m_stealthMode : 1 = false;
    uint m_traceEvents : 1 = false;
    uint m_filterEnabled : 1 = true;
    uint m_filterLocals : 1 = false;
    uint m_filterLocalNet : 1 = false;
    uint m_blockTraffic : 1 = false;
    uint m_blockLanTraffic : 1 = false;
    uint m_blockInetTraffic : 1 = false;
    uint m_allowAllNew : 1 = false;
    uint m_askToConnect : 1 = false;
    uint m_groupBlocked : 1 = true;

    uint m_logStat : 1 = false;
    uint m_logStatNoFilter : 1 = false;
    uint m_logApp : 1 = false;

    uint m_logAllowedConn : 1 = false;
    uint m_logBlockedConn : 1 = false;
    uint m_logAlertedConn : 1 = false;

    uint m_appBlockAll : 1 = true;
    uint m_appAllowAll : 1 = false;

    uint m_activePeriodEnabled : 1 = false;
    uint m_anyGroupPeriodEnabled : 1 = false;

    quint32 m_appGroupBits = 0;
    quint32 m_groupActivePeriodBits = quint32(-1); // transient

    QString m_activePeriodFrom;
    QString m_activePeriodTo;

    QList<AddressGroup *> m_addressGroups;

    QList<AppGroup *> m_appGroups;
    mutable QVector<qint64> m_removedAppGroupIdList;

    IniOptions m_ini;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FirewallConf::EditedFlags)

#endif // FIREWALLCONF_H
