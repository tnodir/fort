#ifndef FIREWALLCONF_H
#define FIREWALLCONF_H

#include <QObject>
#include <QQmlListProperty>
#include <QVariant>

class AppGroup;

class FirewallConf : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool filterDisabled READ filterDisabled WRITE setFilterDisabled NOTIFY filterDisabledChanged)
    Q_PROPERTY(bool ipIncludeAll READ ipIncludeAll WRITE setIpIncludeAll NOTIFY ipIncludeAllChanged)
    Q_PROPERTY(bool ipExcludeAll READ ipExcludeAll WRITE setIpExcludeAll NOTIFY ipExcludeAllChanged)
    Q_PROPERTY(bool appLogBlocked READ appLogBlocked WRITE setAppLogBlocked NOTIFY appLogBlockedChanged)
    Q_PROPERTY(bool appBlockAll READ appBlockAll WRITE setAppBlockAll NOTIFY appBlockAllChanged)
    Q_PROPERTY(bool appAllowAll READ appAllowAll WRITE setAppAllowAll NOTIFY appAllowAllChanged)
    Q_PROPERTY(QString ipIncludeText READ ipIncludeText WRITE setIpIncludeText NOTIFY ipIncludeTextChanged)
    Q_PROPERTY(QString ipExcludeText READ ipExcludeText WRITE setIpExcludeText NOTIFY ipExcludeTextChanged)
    Q_PROPERTY(QQmlListProperty<AppGroup> appGroups READ appGroups NOTIFY appGroupsChanged)
    Q_CLASSINFO("DefaultProperty", "appGroups")

public:
    explicit FirewallConf(QObject *parent = nullptr);

    bool filterDisabled() const { return m_filterDisabled; }
    void setFilterDisabled(bool filterDisabled);

    bool ipIncludeAll() const { return m_ipIncludeAll; }
    void setIpIncludeAll(bool ipIncludeAll);

    bool ipExcludeAll() const { return m_ipExcludeAll; }
    void setIpExcludeAll(bool ipExcludeAll);

    bool appLogBlocked() const { return m_appLogBlocked; }
    void setAppLogBlocked(bool appLogBlocked);

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll);

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll);

    quint32 appGroupBits() const;
    void setAppGroupBits(quint32 groupBits);

    QString ipIncludeText() const { return m_ipIncludeText; }
    void setIpIncludeText(const QString &ipIncludeText);

    QString ipExcludeText() const { return m_ipExcludeText; }
    void setIpExcludeText(const QString &ipExcludeText);

    const QList<AppGroup *> &appGroupsList() const { return m_appGroups; }
    QQmlListProperty<AppGroup> appGroups();

    void addAppGroup(AppGroup *appGroup, int to = -1);
    void moveAppGroup(int from, int to);
    void removeAppGroup(int from, int to);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void filterDisabledChanged();
    void ipIncludeAllChanged();
    void ipExcludeAllChanged();
    void appLogBlockedChanged();
    void appBlockAllChanged();
    void appAllowAllChanged();
    void ipIncludeTextChanged();
    void ipExcludeTextChanged();
    void appGroupsChanged();

public slots:

private:
    uint m_filterDisabled   : 1;

    uint m_ipIncludeAll     : 1;
    uint m_ipExcludeAll     : 1;

    uint m_appLogBlocked    : 1;
    uint m_appBlockAll      : 1;
    uint m_appAllowAll      : 1;

    QString m_ipIncludeText;
    QString m_ipExcludeText;

    QList<AppGroup *> m_appGroups;
};

#endif // FIREWALLCONF_H
