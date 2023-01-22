#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QVector>

#include "addressrange.h"

class AddressGroup;
class App;
class AppGroup;
class ConfAppsWalker;
class EnvManager;
class FirewallConf;

struct fort_traf;

using longs_arr_t = QVector<quint32>;
using shorts_arr_t = QVector<quint16>;
using chars_arr_t = QVector<qint8>;

using addrranges_arr_t = QVarLengthArray<AddressRange, 2>;
using appentry_map_t = QMap<QString, quint32>;

class ConfUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfUtil(QObject *parent = nullptr);

    static int zoneMaxCount();

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

public slots:
    int write(const FirewallConf &conf, ConfAppsWalker *confAppsWalker, EnvManager &envManager,
            QByteArray &buf);
    int writeFlags(const FirewallConf &conf, QByteArray &buf);
    int writeAppEntry(const App &app, bool isNew, QByteArray &buf);
    int writeVersion(QByteArray &buf);
    int writeZone(const IpRange &ipRange, QByteArray &buf);
    int writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData, QByteArray &buf);
    void migrateZoneData(char **data, const QByteArray &zoneData);
    int writeZoneFlag(int zoneId, bool enabled, QByteArray &buf);

    bool loadZone(const QByteArray &buf, IpRange &ipRange);

private:
    void setErrorMessage(const QString &errorMessage);

    bool parseAddressGroups(const QList<AddressGroup *> &addressGroups,
            addrranges_arr_t &addressRanges, longs_arr_t &addressGroupOffsets,
            quint32 &addressGroupsSize);

    // Convert app. groups to plain lists
    bool parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
            chars_arr_t &appPeriods, quint8 &appPeriodsCount, appentry_map_t &wildAppsMap,
            appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap, quint32 &wildAppsSize,
            quint32 &prefixAppsSize, quint32 &exeAppsSize);

    bool parseExeApps(
            ConfAppsWalker *confAppsWalker, appentry_map_t &exeAppsMap, quint32 &exeAppsSize);

    bool parseAppsText(App &app, const QString &text, appentry_map_t &wildAppsMap,
            appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap, quint32 &wildAppsSize,
            quint32 &prefixAppsSize, quint32 &exeAppsSize);

    bool addParsedApp(App &app, bool isWild, bool isPrefix, appentry_map_t &wildAppsMap,
            appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap, quint32 &wildAppsSize,
            quint32 &prefixAppsSize, quint32 &exeAppsSize);

    bool addApp(const App &app, bool isNew, appentry_map_t &appsMap, quint32 &appsSize,
            bool canOverwrite = true);

    static QString parseAppPath(const StringView line, bool &isWild, bool &isPrefix);

    static void parseAppPeriod(
            const AppGroup *appGroup, chars_arr_t &appPeriods, quint8 &appPeriodsCount);

    static void writeConf(char *output, const FirewallConf &conf,
            const addrranges_arr_t &addressRanges, const longs_arr_t &addressGroupOffsets,
            const chars_arr_t &appPeriods, quint8 appPeriodsCount,
            const appentry_map_t &wildAppsMap, const appentry_map_t &prefixAppsMap,
            const appentry_map_t &exeAppsMap);

    static void writeAppGroupFlags(
            quint16 *groupBits, quint16 *logConnBits, const FirewallConf &conf);

    static void writeLimits(struct fort_speed_limit *limits, quint16 *limitBits,
            quint32 *limitIoBits, const QList<AppGroup *> &appGroups);
    static void writeLimit(struct fort_speed_limit *limit, quint32 kiBytes, quint32 bufferSize,
            quint32 latencyMsec, quint16 packetLoss);

    static void writeAddressRanges(char **data, const addrranges_arr_t &addressRanges);
    static void writeAddressRange(char **data, const AddressRange &addressRange);

    static void writeAddressList(char **data, const IpRange &ipRange);
    static void writeAddress4List(char **data, const IpRange &ipRange);
    static void writeAddress6List(char **data, const IpRange &ipRange);

    static bool loadAddressList(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress4List(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress6List(const char **data, IpRange &ipRange, uint &bufSize);

    static void writeApps(char **data, const appentry_map_t &apps, bool useHeader = false);

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
    QString m_errorMessage;
};

#endif // CONFUTIL_H
