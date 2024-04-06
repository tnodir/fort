#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QVector>

#include <util/service/serviceinfo.h>

#include "appparseoptions.h"

class AddressGroup;
class App;
class AppGroup;
class ConfAppsWalker;
class EnvManager;
class FirewallConf;

using longs_arr_t = QVector<quint32>;
using shorts_arr_t = QVector<quint16>;
using chars_arr_t = QVector<qint8>;

class ConfUtil : public QObject
{
    Q_OBJECT

public:
    explicit ConfUtil(QObject *parent = nullptr);

    quint32 driveMask() const { return m_driveMask; }

    QString errorMessage() const { return m_errorMessage; }

    static int ruleMaxCount();
    static int zoneMaxCount();

public slots:
    int writeVersion(QByteArray &buf);
    int writeServices(
            const QVector<ServiceInfo> &services, int runningServicesCount, QByteArray &buf);
    int write(const FirewallConf &conf, ConfAppsWalker *confAppsWalker, EnvManager &envManager,
            QByteArray &buf);
    int writeFlags(const FirewallConf &conf, QByteArray &buf);
    int writeAppEntry(const App &app, bool isNew, QByteArray &buf);
    int writeZone(const IpRange &ipRange, QByteArray &buf);
    int writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData, QByteArray &buf);
    void migrateZoneData(char **data, const QByteArray &zoneData);
    int writeZoneFlag(int zoneId, bool enabled, QByteArray &buf);

    bool loadZone(const QByteArray &buf, IpRange &ipRange);

private:
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }

    bool parseAddressGroups(const QList<AddressGroup *> &addressGroups,
            addrranges_arr_t &addressRanges, longs_arr_t &addressGroupOffsets,
            quint32 &addressGroupsSize);

    // Convert app. groups to plain lists
    bool parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
            chars_arr_t &appPeriods, quint8 &appPeriodsCount, AppParseOptions &opt);

    bool parseExeApps(EnvManager &envManager, ConfAppsWalker *confAppsWalker, AppParseOptions &opt);

    bool parseAppsText(EnvManager &envManager, App &app, AppParseOptions &opt);

    bool parseAppLine(App &app, const QStringView &line, AppParseOptions &opt);

    bool addApp(const App &app, bool isNew, appdata_map_t &appsMap, quint32 &appsSize);

    static QString parseAppPath(const QStringView line, bool &isWild, bool &isPrefix);

    static void parseAppPeriod(
            const AppGroup *appGroup, chars_arr_t &appPeriods, quint8 &appPeriodsCount);

    static void writeConf(char *output, const FirewallConf &conf,
            const addrranges_arr_t &addressRanges, const longs_arr_t &addressGroupOffsets,
            const chars_arr_t &appPeriods, quint8 appPeriodsCount, AppParseOptions &opt);

    static void writeAppGroupFlags(quint16 *groupBits, quint16 *logBlockedBits,
            quint16 *logConnBits, const FirewallConf &conf);

    static void writeLimits(struct fort_speed_limit *limits, quint16 *limitBits,
            quint32 *limitIoBits, const QList<AppGroup *> &appGroups);
    static void writeLimit(struct fort_speed_limit *limit, quint32 kBits, quint32 bufferSize,
            quint32 latencyMsec, quint16 packetLoss);

    static void writeAddressRanges(char **data, const addrranges_arr_t &addressRanges);
    static void writeAddressRange(char **data, const AddressRange &addressRange);

    static void writeAddressList(char **data, const IpRange &ipRange);
    static void writeAddress4List(char **data, const IpRange &ipRange);
    static void writeAddress6List(char **data, const IpRange &ipRange);

    static bool loadAddressList(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress4List(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress6List(const char **data, IpRange &ipRange, uint &bufSize);

    static void writeApps(char **data, const appdata_map_t &appsMap, bool useHeader = false);

    static void writeShorts(char **data, const shorts_arr_t &array);
    static void writeLongs(char **data, const longs_arr_t &array);
    static void writeIp6Array(char **data, const ip6_arr_t &array);
    static void writeData(char **data, void const *src, int elemCount, uint elemSize);
    static void writeChars(char **data, const chars_arr_t &array);
    static void writeArray(char **data, const QByteArray &array);

    static void loadLongs(const char **data, longs_arr_t &array);
    static void loadIp6Array(const char **data, ip6_arr_t &array);
    static void loadData(const char **data, void *dst, int elemCount, uint elemSize);

private:
    quint32 m_driveMask = 0;

    QString m_errorMessage;
};

#endif // CONFUTIL_H
