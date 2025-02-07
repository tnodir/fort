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
quint32 ioctlSetGroupsMask();
quint32 ioctlGetLog();
quint32 ioctlAddApp();
quint32 ioctlDelApp();
quint32 ioctlSetZones();
quint32 ioctlSetZoneFlag();
quint32 ioctlSetRules();
quint32 ioctlSetRuleFlag();
quint32 ioctlSpeedLimits();
quint32 ioctlSetSpeedLimitFlag();

quint32 userErrorCode();

qint64 systemToUnixTime(qint64 systemTime);

int bufferSize();

quint32 confIoConfOff();

quint32 logAppHeaderSize();
quint32 logAppSize(quint32 pathLen);

quint32 logConnHeaderSize(bool isIPv6 = false);
quint32 logConnSize(quint32 pathLen, bool isIPv6 = false);

quint32 logProcNewHeaderSize();
quint32 logProcNewSize(quint32 pathLen);

quint32 logStatHeaderSize();
quint32 logStatTrafSize(quint16 procCount);
quint32 logStatSize(quint16 procCount);

quint32 logTimeSize();

quint8 logType(const char *input);

void logAppHeaderWrite(char *output, bool blocked, quint32 pid, quint32 pathLen);
void logAppHeaderRead(const char *input, int *blocked, quint32 *pid, quint32 *pathLen);

void logConnHeaderWrite(char *output, PCFORT_CONF_META_CONN conn, quint32 pathLen);
void logConnHeaderRead(const char *input, PFORT_CONF_META_CONN conn, quint32 *pathLen);

void logProcNewHeaderWrite(char *output, quint32 pid, quint32 pathLen);
void logProcNewHeaderRead(const char *input, quint32 *pid, quint32 *pathLen);

void logStatTrafHeaderRead(const char *input, quint16 *procCount);

void logTimeWrite(char *output, int systemTimeChanged, qint64 unixTime);
void logTimeRead(const char *input, int *systemTimeChanged, qint64 *unixTime);

bool confIpInRange(const void *drvConf, const ip_addr_t ip, bool isIPv6 = false,
        bool included = false, int addrGroupIndex = 0);
bool confIp4InRange(const void *drvConf, quint32 ip, bool included = false, int addrGroupIndex = 0);
bool confIp6InRange(
        const void *drvConf, const ip6_addr_t ip, bool included = false, int addrGroupIndex = 0);

FORT_APP_DATA confAppFind(const void *drvConf, const QString &kernelPath);

bool confRulesConnFiltered(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId);
bool confRulesConnBlocked(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId);

bool provRegister(bool bootFilter);
void provUnregister();

}

#endif // DRIVERCOMMON_H
