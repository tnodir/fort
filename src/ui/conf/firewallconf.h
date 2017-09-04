#ifndef FIREWALLCONF_H
#define FIREWALLCONF_H

#include <QObject>
#include <QQmlListProperty>
#include <QVariant>

class AddressGroup;
class AppGroup;

class FirewallConf : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool filterEnabled READ filterEnabled WRITE setFilterEnabled NOTIFY filterEnabledChanged)
    Q_PROPERTY(bool appLogBlocked READ appLogBlocked WRITE setAppLogBlocked NOTIFY appLogBlockedChanged)
    Q_PROPERTY(bool appBlockAll READ appBlockAll WRITE setAppBlockAll NOTIFY appBlockAllChanged)
    Q_PROPERTY(bool appAllowAll READ appAllowAll WRITE setAppAllowAll NOTIFY appAllowAllChanged)
    Q_PROPERTY(AddressGroup *ipInclude READ ipInclude CONSTANT)
    Q_PROPERTY(AddressGroup *ipExclude READ ipExclude CONSTANT)
    Q_PROPERTY(QQmlListProperty<AppGroup> appGroups READ appGroups NOTIFY appGroupsChanged)
    Q_CLASSINFO("DefaultProperty", "appGroups")

public:
    explicit FirewallConf(QObject *parent = nullptr);

    bool filterEnabled() const { return m_filterEnabled; }
    void setFilterEnabled(bool filterEnabled);

    bool appLogBlocked() const { return m_appLogBlocked; }
    void setAppLogBlocked(bool appLogBlocked);

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll);

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll);

    quint32 appGroupBits() const;
    void setAppGroupBits(quint32 groupBits);

    AddressGroup *ipInclude() const { return m_ipInclude; }
    AddressGroup *ipExclude() const { return m_ipExclude; }

    const QList<AppGroup *> &appGroupsList() const { return m_appGroups; }
    QQmlListProperty<AppGroup> appGroups();

    void copyFlags(const FirewallConf &o);
    void copyTempFlags(const FirewallConf &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void filterEnabledChanged();
    void appLogBlockedChanged();
    void appBlockAllChanged();
    void appAllowAllChanged();
    void appGroupsChanged();

public slots:
    void addAppGroup(AppGroup *appGroup, int to = -1);
    void addAppGroupByName(const QString &name);
    void moveAppGroup(int from, int to);
    void removeAppGroup(int from, int to);

private:
    uint m_filterEnabled    : 1;

    uint m_appLogBlocked    : 1;  // transient

    uint m_appBlockAll      : 1;
    uint m_appAllowAll      : 1;

    AddressGroup *m_ipInclude;
    AddressGroup *m_ipExclude;

    QList<AppGroup *> m_appGroups;
};

#endif // FIREWALLCONF_H
