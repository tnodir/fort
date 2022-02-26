#include "drivercommon.h"

#include <common/fortconf.h>
#include <common/fortioctl.h>
#include <common/fortlog.h>
#include <common/fortprov.h>

namespace DriverCommon {

QString deviceName()
{
    return QLatin1String(FORT_DEVICE_NAME);
}

quint32 ioctlValidate()
{
    return FORT_IOCTL_VALIDATE;
}

quint32 ioctlSetConf()
{
    return FORT_IOCTL_SETCONF;
}

quint32 ioctlSetFlags()
{
    return FORT_IOCTL_SETFLAGS;
}

quint32 ioctlGetLog()
{
    return FORT_IOCTL_GETLOG;
}

quint32 ioctlAddApp()
{
    return FORT_IOCTL_ADDAPP;
}

quint32 ioctlDelApp()
{
    return FORT_IOCTL_DELAPP;
}

quint32 ioctlSetZones()
{
    return FORT_IOCTL_SETZONES;
}

quint32 ioctlSetZoneFlag()
{
    return FORT_IOCTL_SETZONEFLAG;
}

quint32 userErrorCode()
{
    return FORT_ERROR_USER_ERROR;
}

qint64 systemToUnixTime(qint64 systemTime)
{
    return fort_system_to_unix_time(systemTime);
}

int bufferSize()
{
    return FORT_BUFFER_SIZE;
}

quint32 confIoConfOff()
{
    return FORT_CONF_IO_CONF_OFF;
}

quint32 logBlockedHeaderSize()
{
    return FORT_LOG_BLOCKED_HEADER_SIZE;
}

quint32 logBlockedSize(quint32 pathLen)
{
    return FORT_LOG_BLOCKED_SIZE(pathLen);
}

quint32 logBlockedIpHeaderSize()
{
    return FORT_LOG_BLOCKED_IP_HEADER_SIZE;
}

quint32 logBlockedIpSize(quint32 pathLen)
{
    return FORT_LOG_BLOCKED_IP_SIZE(pathLen);
}

quint32 logProcNewHeaderSize()
{
    return FORT_LOG_PROC_NEW_HEADER_SIZE;
}

quint32 logProcNewSize(quint32 pathLen)
{
    return FORT_LOG_PROC_NEW_SIZE(pathLen);
}

quint32 logStatHeaderSize()
{
    return FORT_LOG_STAT_HEADER_SIZE;
}

quint32 logStatTrafSize(quint16 procCount)
{
    return FORT_LOG_STAT_TRAF_SIZE(procCount);
}

quint32 logStatSize(quint16 procCount)
{
    return FORT_LOG_STAT_SIZE(procCount);
}

quint32 logTimeSize()
{
    return FORT_LOG_TIME_SIZE;
}

quint8 logType(const char *input)
{
    return fort_log_type(input);
}

void logBlockedHeaderWrite(char *output, bool blocked, quint32 pid, quint32 pathLen)
{
    fort_log_blocked_header_write(output, blocked, pid, pathLen);
}

void logBlockedHeaderRead(const char *input, int *blocked, quint32 *pid, quint32 *pathLen)
{
    fort_log_blocked_header_read(input, blocked, pid, pathLen);
}

void logBlockedIpHeaderWrite(char *output, int inbound, int inherited, quint8 blockReason,
        quint8 ipProto, quint16 localPort, quint16 remotePort, quint32 localIp, quint32 remoteIp,
        quint32 pid, quint32 pathLen)
{
    fort_log_blocked_ip_header_write(output, inbound, inherited, blockReason, ipProto, localPort,
            remotePort, localIp, remoteIp, pid, pathLen);
}

void logBlockedIpHeaderRead(const char *input, int *inbound, int *inherited, quint8 *blockReason,
        quint8 *ipProto, quint16 *localPort, quint16 *remotePort, quint32 *localIp,
        quint32 *remoteIp, quint32 *pid, quint32 *pathLen)
{
    fort_log_blocked_ip_header_read(input, inbound, inherited, blockReason, ipProto, localPort,
            remotePort, localIp, remoteIp, pid, pathLen);
}

void logProcNewHeaderWrite(char *output, quint32 pid, quint32 pathLen)
{
    fort_log_proc_new_header_write(output, pid, pathLen);
}

void logProcNewHeaderRead(const char *input, quint32 *pid, quint32 *pathLen)
{
    fort_log_proc_new_header_read(input, pid, pathLen);
}

void logStatTrafHeaderRead(const char *input, quint16 *procCount)
{
    fort_log_stat_traf_header_read(input, procCount);
}

void logTimeWrite(char *output, int timeChanged, qint64 unixTime)
{
    fort_log_time_write(output, timeChanged, unixTime);
}

void logTimeRead(const char *input, int *timeChanged, qint64 *unixTime)
{
    fort_log_time_read(input, timeChanged, unixTime);
}

void confAppPermsMaskInit(void *drvConf)
{
    PFORT_CONF conf = (PFORT_CONF) drvConf;

    fort_conf_app_perms_mask_init(conf, conf->flags.group_bits);
}

bool confIpInRange(const void *drvConf, quint32 ip, bool included, int addrGroupIndex)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const PFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addrGroupIndex);

    const bool is_empty = included ? addr_group->include_is_empty : addr_group->exclude_is_empty;
    if (is_empty)
        return false;

    const PFORT_CONF_ADDR_LIST addr_list = included
            ? fort_conf_addr_group_include_list_ref(addr_group)
            : fort_conf_addr_group_exclude_list_ref(addr_group);

    return fort_conf_ip_inlist(ip, addr_list);
}

quint16 confAppFind(const void *drvConf, const QString &kernelPath)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const QString kernelPathLower = kernelPath.toLower();
    const quint32 len = quint32(kernelPathLower.size()) * sizeof(WCHAR);
    const WCHAR *p = (PCWCHAR) kernelPathLower.utf16();

    const FORT_APP_FLAGS app_flags =
            fort_conf_app_find(conf, (const PVOID) p, len, fort_conf_app_exe_find);

    return app_flags.v;
}

quint8 confAppGroupIndex(quint16 appFlags)
{
    const FORT_APP_FLAGS app_flags = { appFlags };

    return app_flags.group_index;
}

bool confAppBlocked(const void *drvConf, quint16 appFlags, qint8 *blockReason)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    return fort_conf_app_blocked(conf, { appFlags }, blockReason);
}

quint16 confAppPeriodBits(const void *drvConf, quint8 hour, quint8 minute)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    FORT_TIME time;
    time.hour = hour;
    time.minute = minute;

    return fort_conf_app_period_bits(conf, time, nullptr);
}

bool isTimeInPeriod(quint8 hour, quint8 minute, quint8 fromHour, quint8 fromMinute, quint8 toHour,
        quint8 toMinute)
{
    FORT_TIME time;
    time.hour = hour;
    time.minute = minute;

    FORT_PERIOD period;
    period.from.hour = fromHour;
    period.from.minute = fromMinute;
    period.to.hour = toHour;
    period.to.minute = toMinute;

    return is_time_in_period(time, period);
}

int bitScanForward(quint32 mask)
{
    return bit_scan_forward(mask);
}

void provUnregister()
{
    fort_prov_unregister(nullptr);
}

}
