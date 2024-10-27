#ifndef CONFBUFFER_H
#define CONFBUFFER_H

#include <QByteArray>

#include <util/conf/confappswalker.h>
#include <util/conf/confruleswalker.h>
#include <util/service/serviceinfo.h>

#include "confutil.h"

class AddressGroup;
class AppGroup;
class EnvManager;

class ConfBuffer : public QObject
{
    Q_OBJECT

public:
    explicit ConfBuffer(const QByteArray &buffer = {}, QObject *parent = nullptr);

    quint32 driveMask() const { return m_driveMask; }

    QString errorMessage() const { return m_errorMessage; }

    bool hasError() const { return !errorMessage().isEmpty(); }

    const QByteArray &buffer() const { return m_buffer; }
    QByteArray &buffer() { return m_buffer; }

    const char *data() const { return buffer().constData(); }
    char *data() { return m_buffer.data(); }

public slots:
    void writeVersion();

    void writeServices(const QVector<ServiceInfo> &services, int runningServicesCount);
    void writeServiceSids(const QVector<ServiceInfo> &services);

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

    bool parseAddressGroups(const QList<AddressGroup *> &addressGroups, ParseAddressGroupsArgs &ad,
            quint32 &addressGroupsSize);

    bool parseAppGroups(
            EnvManager &envManager, const QList<AppGroup *> &appGroups, AppParseOptions &opt);

    bool parseExeApps(
            EnvManager &envManager, const ConfAppsWalker *confAppsWalker, AppParseOptions &opt);

    bool parseAppsText(EnvManager &envManager, App &app, AppParseOptions &opt);

    bool parseAppLine(App &app, const QStringView &line, AppParseOptions &opt);

    bool addApp(const App &app, bool isNew, appdata_map_t &appsMap, quint32 &appsSize);

    void writeRule(
            const Rule &rule, const ruleset_map_t &ruleSetMap, const ruleid_arr_t &ruleSetIds);
    void writeRuleText(const QString &ruleText);

private:
    quint32 m_driveMask = 0;

    QString m_errorMessage;

    QByteArray m_buffer;
};

#endif // CONFBUFFER_H
