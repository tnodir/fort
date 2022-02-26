#ifndef DRIVERCOMMON_H
#define DRIVERCOMMON_H

#include <QString>

namespace DriverCommon {

QString deviceName();

quint32 ioctlValidate();
quint32 ioctlSetConf();
quint32 ioctlSetFlags();
quint32 ioctlGetLog();
quint32 ioctlAddApp();
quint32 ioctlDelApp();
quint32 ioctlSetZones();
quint32 ioctlSetZoneFlag();

quint32 userErrorCode();

qint64 systemToUnixTime(qint64 systemTime);

int bufferSize();

quint32 confIoConfOff();

quint32 logBlockedHeaderSize();
quint32 logBlockedSize(quint32 pathLen);

quint32 logBlockedIpHeaderSize();
quint32 logBlockedIpSize(quint32 pathLen);

quint32 logProcNewHeaderSize();
quint32 logProcNewSize(quint32 pathLen);

quint32 logStatHeaderSize();
quint32 logStatTrafSize(quint16 procCount);
quint32 logStatSize(quint16 procCount);

quint32 logTimeSize();

quint8 logType(const char *input);

void logBlockedHeaderWrite(char *output, bool blocked, quint32 pid, quint32 pathLen);
void logBlockedHeaderRead(const char *input, int *blocked, quint32 *pid, quint32 *pathLen);

void logBlockedIpHeaderWrite(char *output, int inbound, int inherited, quint8 blockReason,
        quint8 ipProto, quint16 localPort, quint16 remotePort, quint32 localIp, quint32 remoteIp,
        quint32 pid, quint32 pathLen);
void logBlockedIpHeaderRead(const char *input, int *inbound, int *inherited, quint8 *blockReason,
        quint8 *ipProto, quint16 *localPort, quint16 *remotePort, quint32 *localIp,
        quint32 *remoteIp, quint32 *pid, quint32 *pathLen);

void logProcNewHeaderWrite(char *output, quint32 pid, quint32 pathLen);
void logProcNewHeaderRead(const char *input, quint32 *pid, quint32 *pathLen);

void logStatTrafHeaderRead(const char *input, quint16 *procCount);

void logTimeWrite(char *output, int timeChanged, qint64 unixTime);
void logTimeRead(const char *input, int *timeChanged, qint64 *unixTime);

void confAppPermsMaskInit(void *drvConf);
bool confIpInRange(const void *drvConf, quint32 ip, bool included = false, int addrGroupIndex = 0);
quint16 confAppFind(const void *drvConf, const QString &kernelPath);
quint8 confAppGroupIndex(quint16 appFlags);
bool confAppBlocked(const void *drvConf, quint16 appFlags, qint8 *blockReason);
quint16 confAppPeriodBits(const void *drvConf, quint8 hour, quint8 minute);

bool isTimeInPeriod(quint8 hour, quint8 minute, quint8 fromHour, quint8 fromMinute, quint8 toHour,
        quint8 toMinute);

int bitScanForward(quint32 mask);

void provUnregister();

}

#endif // DRIVERCOMMON_H
