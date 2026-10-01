#ifndef DRIVERCOMMON_H
#define DRIVERCOMMON_H

#include <QObject>

#include <common/fortconf.h>

namespace DriverCommon {

QString deviceName();

quint32 ioctlValidate();
quint32 ioctlSetServices();
quint32 ioctlSetConf();
quint32 ioctlSetFlags();
quint32 ioctlGetLog();
quint32 ioctlAddApp();
quint32 ioctlDelApp();
quint32 ioctlSetZones();
quint32 ioctlSetZoneFlag();
quint32 ioctlSetRules();
quint32 ioctlSetRuleFlag();
quint32 ioctlSetGroups();
quint32 ioctlSetGroupFlags();
quint32 ioctlSetSpeedLimits();
quint32 ioctlSetSpeedLimitFlags();

quint32 userErrorCode();

qint64 systemToUnixTime(qint64 systemTime);

int bufferSize();

quint32 confIoConfOff();

quint32 logAppHeaderSize();
quint32 logAppSize(quint16 pathLen);

quint32 logConnHeaderSize(bool isIPv6 = false);
quint32 logConnSize(quint16 pathLen, bool isIPv6 = false);

quint32 logProcNewHeaderSize();
quint32 logProcNewSize(quint16 pathLen);

quint32 logStatHeaderSize();
quint32 logStatTrafSize(quint16 procCount);
quint32 logStatSize(quint16 procCount);

quint32 logTimeSize();

quint32 logProcKillSize();

quint8 logType(const char *input);

void logAppHeaderWrite(char *output, bool blocked, quint32 pid, quint16 pathLen);
void logAppHeaderRead(const char *input, int *blocked, quint32 *pid, quint16 *pathLen);

void logConnHeaderWrite(char *output, PCFORT_CONF_META_CONN conn, quint16 pathLen);
void logConnHeaderRead(const char *input, PFORT_CONF_META_CONN conn, quint16 *pathLen);

void logProcNewHeaderWrite(char *output, quint32 appId, quint32 pid, quint16 pathLen);
void logProcNewHeaderRead(const char *input, quint32 *appId, quint32 *pid, quint16 *pathLen);

void logStatTrafHeaderRead(const char *input, quint16 *procCount);

void logTimeWrite(char *output, qint64 unixTime, int systemTimeChanged);
void logTimeRead(const char *input, qint64 *unixTime, int *systemTimeChanged);

void logProcKillWrite(char *output, quint32 pid);
void logProcKillRead(const char *input, quint32 *pid);

bool confIpInRange(const void *drvConf, const ip_addr_t ip, bool isIPv6 = false,
        bool included = false, int addrGroupIndex = 0);
bool confIp4InRange(const void *drvConf, quint32 ip, bool included = false, int addrGroupIndex = 0);
bool confIp6InRange(
        const void *drvConf, const ip6_addr_t ip, bool included = false, int addrGroupIndex = 0);

FORT_APP_DATA confAppFind(const void *drvConf, const QString &appPath);

bool wildMatch(const QString &pattern, const QString &text);
bool wildMatchPath(const QString &pattern, const QString &path);

bool confGroupsMaskBlocked(const void *drvGroups, quint32 groupsMask);
quint16 confGroupsRulesConnFiltered(
        const void *drvGroups, const void *drvRules, PFORT_CONF_META_CONN conn, quint32 groupsMask);

bool confRulesConnFiltered(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId,
        const void *drvZones = nullptr);
bool confRulesConnBlocked(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId);

bool confIoValid(const void *drvConfIo, quint32 len);
bool confZonesValid(const void *drvZones, quint32 len);
bool confRulesValid(const void *drvRules, quint32 len);

bool provRegister(bool bootFilter);
void provUnregister();

}

#endif // DRIVERCOMMON_H
