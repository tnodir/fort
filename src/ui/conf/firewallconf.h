#ifndef FIREWALLCONF_H
#define FIREWALLCONF_H

#include <QObject>
#include <QQmlListProperty>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(AddressGroup)
QT_FORWARD_DECLARE_CLASS(AppGroup)

#define DEFAULT_APP_GROUP_BITS          0xFFFF
#define DEFAULT_MONTH_START             1
#define DEFAULT_TRAF_HOUR_KEEP_DAYS     90  // ~3 months
#define DEFAULT_TRAF_DAY_KEEP_DAYS      365  // ~1 year
#define DEFAULT_TRAF_MONTH_KEEP_MONTHS  36  // ~3 years

class FirewallConf : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool provBoot READ provBoot WRITE setProvBoot NOTIFY provBootChanged)
    Q_PROPERTY(bool filterEnabled READ filterEnabled WRITE setFilterEnabled NOTIFY filterEnabledChanged)
    Q_PROPERTY(bool stopTraffic READ stopTraffic WRITE setStopTraffic NOTIFY stopTrafficChanged)
    Q_PROPERTY(bool stopInetTraffic READ stopInetTraffic WRITE setStopInetTraffic NOTIFY stopInetTrafficChanged)
    Q_PROPERTY(bool resolveAddress READ resolveAddress WRITE setResolveAddress NOTIFY resolveAddressChanged)
    Q_PROPERTY(bool logBlocked READ logBlocked WRITE setLogBlocked NOTIFY logBlockedChanged)
    Q_PROPERTY(bool logStat READ logStat WRITE setLogStat NOTIFY logStatChanged)
    Q_PROPERTY(bool appBlockAll READ appBlockAll WRITE setAppBlockAll NOTIFY appBlockAllChanged)
    Q_PROPERTY(bool appAllowAll READ appAllowAll WRITE setAppAllowAll NOTIFY appAllowAllChanged)
    Q_PROPERTY(bool activePeriodEnabled READ activePeriodEnabled WRITE setActivePeriodEnabled NOTIFY activePeriodEnabledChanged)
    Q_PROPERTY(int activePeriodFrom READ activePeriodFrom WRITE setActivePeriodFrom NOTIFY activePeriodFromChanged)
    Q_PROPERTY(int activePeriodTo READ activePeriodTo WRITE setActivePeriodTo NOTIFY activePeriodToChanged)
    Q_PROPERTY(int monthStart READ monthStart WRITE setMonthStart NOTIFY monthStartChanged)
    Q_PROPERTY(int trafHourKeepDays READ trafHourKeepDays WRITE setTrafHourKeepDays NOTIFY trafHourKeepDaysChanged)
    Q_PROPERTY(int trafDayKeepDays READ trafDayKeepDays WRITE setTrafDayKeepDays NOTIFY trafDayKeepDaysChanged)
    Q_PROPERTY(int trafMonthKeepMonths READ trafMonthKeepMonths WRITE setTrafMonthKeepMonths NOTIFY trafMonthKeepMonthsChanged)
    Q_PROPERTY(int trafUnit READ trafUnit WRITE setTrafUnit NOTIFY trafUnitChanged)
    Q_PROPERTY(quint32 quotaDayMb READ quotaDayMb WRITE setQuotaDayMb NOTIFY quotaDayMbChanged)
    Q_PROPERTY(quint32 quotaMonthMb READ quotaMonthMb WRITE setQuotaMonthMb NOTIFY quotaMonthMbChanged)
    Q_PROPERTY(bool hasPassword READ hasPassword NOTIFY passwordHashChanged)
    Q_PROPERTY(QString passwordHash READ passwordHash WRITE setPasswordHash NOTIFY passwordHashChanged)
    Q_PROPERTY(AddressGroup *inetAddressGroup READ inetAddressGroup NOTIFY addressGroupsChanged)
    Q_PROPERTY(QQmlListProperty<AddressGroup> addressGroups READ addressGroups NOTIFY addressGroupsChanged)
    Q_PROPERTY(QQmlListProperty<AppGroup> appGroups READ appGroups NOTIFY appGroupsChanged)
    Q_CLASSINFO("DefaultProperty", "appGroups")

public:
    enum TrafUnit {
        UnitAdaptive = 0,
        UnitBytes,
        UnitKB,
        UnitMB,
        UnitGB,
        UnitTB
    };
    Q_ENUM(TrafUnit)

    explicit FirewallConf(QObject *parent = nullptr);

    bool provBoot() const { return m_provBoot; }
    void setProvBoot(bool provBoot);

    bool filterEnabled() const { return m_filterEnabled; }
    void setFilterEnabled(bool filterEnabled);

    bool stopTraffic() const { return m_stopTraffic; }
    void setStopTraffic(bool stopTraffic);

    bool stopInetTraffic() const { return m_stopInetTraffic; }
    void setStopInetTraffic(bool stopInetTraffic);

    bool resolveAddress() const { return m_resolveAddress; }
    void setResolveAddress(bool resolveAddress);

    bool logBlocked() const { return m_logBlocked; }
    void setLogBlocked(bool logBlocked);

    bool logStat() const { return m_logStat; }
    void setLogStat(bool logStat);

    bool appBlockAll() const { return m_appBlockAll; }
    void setAppBlockAll(bool appBlockAll);

    bool appAllowAll() const { return m_appAllowAll; }
    void setAppAllowAll(bool appAllowAll);

    bool activePeriodEnabled() const { return m_activePeriodEnabled; }
    void setActivePeriodEnabled(bool activePeriodEnabled);

    int activePeriodFrom() const { return m_activePeriodFrom; }
    void setActivePeriodFrom(int activePeriodFrom);

    int activePeriodTo() const { return m_activePeriodTo; }
    void setActivePeriodTo(int activePeriodTo);

    int monthStart() const { return m_monthStart; }
    void setMonthStart(int monthStart);

    int trafHourKeepDays() const { return m_trafHourKeepDays; }
    void setTrafHourKeepDays(int trafHourKeepDays);

    int trafDayKeepDays() const { return m_trafDayKeepDays; }
    void setTrafDayKeepDays(int trafDayKeepDays);

    int trafMonthKeepMonths() const { return m_trafMonthKeepMonths; }
    void setTrafMonthKeepMonths(int trafMonthKeepMonths);

    int trafUnit() const { return m_trafUnit; }
    void setTrafUnit(int trafUnit);

    quint32 quotaDayMb() const { return m_quotaDayMb; }
    void setQuotaDayMb(quint32 quotaDayMb);

    quint32 quotaMonthMb() const { return m_quotaMonthMb; }
    void setQuotaMonthMb(quint32 quotaMonthMb);

    bool hasPassword() const { return !m_passwordHash.isEmpty(); }

    QString passwordHash() const { return m_passwordHash; }
    void setPasswordHash(const QString &passwordHash);

    quint32 appGroupBits() const;
    void setAppGroupBits(quint32 groupBits);

    AddressGroup *inetAddressGroup() const { return m_addressGroups.at(0); }

    const QList<AddressGroup *> &addressGroupsList() const { return m_addressGroups; }
    QQmlListProperty<AddressGroup> addressGroups();

    Q_INVOKABLE AppGroup *appGroupByName(const QString &name) const;

    const QList<AppGroup *> &appGroupsList() const { return m_appGroups; }
    QQmlListProperty<AppGroup> appGroups();

    void copyFlags(const FirewallConf &o);
    void copyImmediateFlags(const FirewallConf &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

    void setupDefault();

signals:
    void provBootChanged();
    void filterEnabledChanged();
    void stopTrafficChanged();
    void stopInetTrafficChanged();
    void resolveAddressChanged();
    void logBlockedChanged();
    void logStatChanged();
    void appBlockAllChanged();
    void appAllowAllChanged();
    void activePeriodEnabledChanged();
    void activePeriodFromChanged();
    void activePeriodToChanged();
    void monthStartChanged();
    void trafHourKeepDaysChanged();
    void trafDayKeepDaysChanged();
    void trafMonthKeepMonthsChanged();
    void trafUnitChanged();
    void quotaDayMbChanged();
    void quotaMonthMbChanged();
    void passwordHashChanged();
    void addressGroupsChanged();
    void appGroupsChanged();

public slots:
    void addAppGroup(AppGroup *appGroup, int to = -1);
    void addAppGroupByName(const QString &name);
    void moveAppGroup(int from, int to);
    void removeAppGroup(int from, int to);

private:
    uint m_provBoot         : 1;
    uint m_filterEnabled    : 1;
    uint m_stopTraffic      : 1;
    uint m_stopInetTraffic  : 1;

    uint m_resolveAddress   : 1;

    uint m_logBlocked       : 1;
    uint m_logStat          : 1;

    uint m_appBlockAll      : 1;
    uint m_appAllowAll      : 1;

    uint m_activePeriodEnabled : 1;
    uint m_activePeriodFrom : 5;
    uint m_activePeriodTo   : 5;

    uint m_monthStart       : 5;

    int m_trafHourKeepDays;
    int m_trafDayKeepDays;
    int m_trafMonthKeepMonths;

    TrafUnit m_trafUnit;

    quint32 m_quotaDayMb;
    quint32 m_quotaMonthMb;

    QString m_passwordHash;

    QList<AddressGroup *> m_addressGroups;
    QList<AppGroup *> m_appGroups;
};

#endif // FIREWALLCONF_H
