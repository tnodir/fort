#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QRegularExpressionMatch>
#include <QVector>

#include <util/service/serviceinfo.h>

#include "appparseoptions.h"

class AddressGroup;
class App;
class AppGroup;
class ConfAppsWalker;
class ConfRulesWalker;
class EnvManager;
class FirewallConf;

using longs_arr_t = QVector<quint32>;
using shorts_arr_t = QVector<quint16>;
using chars_arr_t = QVector<qint8>;

class ConfUtil : public QObject
{
    Q_OBJECT

public:
    explicit ConfUtil(const QByteArray &buffer = {}, QObject *parent = nullptr);

    quint32 driveMask() const { return m_driveMask; }

    QString errorMessage() const { return m_errorMessage; }

    bool hasError() const { return !errorMessage().isEmpty(); }

    const QByteArray &buffer() const { return m_buffer; }
    QByteArray &buffer() { return m_buffer; }

    const char *data() const { return buffer().constData(); }
    char *data() { return m_buffer.data(); }

    static int ruleMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthMaxCount();
    static int ruleSetDepthMaxCount();

    static int zoneMaxCount();

    static QRegularExpressionMatch matchWildcard(const QStringView &path);

public slots:
    void writeVersion();
    void writeServices(const QVector<ServiceInfo> &services, int runningServicesCount);

    bool write(
            const FirewallConf &conf, const ConfAppsWalker *confAppsWalker, EnvManager &envManager);
    void writeFlags(const FirewallConf &conf);
    bool writeAppEntry(const App &app, bool isNew = false);

    bool writeRules(const ConfRulesWalker &confRulesWalker);

    void writeZone(const IpRange &ipRange);
    void writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);
    void writeZoneFlag(int zoneId, bool enabled);

    bool loadZone(IpRange &ipRange);

private:
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }

    struct ParseAddressGroupsArgs
    {
        addrranges_arr_t addressRanges;
        longs_arr_t addressGroupOffsets;
    };

    struct ParseAppGroupsArgs
    {
        chars_arr_t appPeriods;
        quint8 appPeriodsCount = 0;
    };

    struct WriteConfArgs
    {
        const FirewallConf &conf;

        ParseAddressGroupsArgs ad;
        ParseAppGroupsArgs gr;
    };

    bool parseAddressGroups(const QList<AddressGroup *> &addressGroups, ParseAddressGroupsArgs &ad,
            quint32 &addressGroupsSize);

    // Convert app. groups to plain lists
    bool parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
            ParseAppGroupsArgs &gr, AppParseOptions &opt);

    bool parseExeApps(
            EnvManager &envManager, const ConfAppsWalker *confAppsWalker, AppParseOptions &opt);

    bool parseAppsText(EnvManager &envManager, App &app, AppParseOptions &opt);

    bool parseAppLine(App &app, const QStringView &line, AppParseOptions &opt);

    bool addApp(const App &app, bool isNew, appdata_map_t &appsMap, quint32 &appsSize);

    static QString parseAppPath(const QStringView &line, bool &isWild, bool &isPrefix);

    static void parseAppPeriod(const AppGroup *appGroup, ParseAppGroupsArgs &gr);

    static void writeConf(char *output, const WriteConfArgs &wca, AppParseOptions &opt);

    static void writeAddressRanges(char **data, const addrranges_arr_t &addressRanges);
    static void writeAddressRange(char **data, const AddressRange &addressRange);

    static void writeAddressList(char **data, const IpRange &ipRange);
    static void writeAddress4List(char **data, const IpRange &ipRange);
    static void writeAddress6List(char **data, const IpRange &ipRange);

    static bool loadAddressList(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress4List(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadAddress6List(const char **data, IpRange &ipRange, uint &bufSize);

    static void writeApps(char **data, const appdata_map_t &appsMap, bool useHeader = false);

    static void migrateZoneData(char **data, const QByteArray &zoneData);

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

    QByteArray m_buffer;
};

#endif // CONFUTIL_H
