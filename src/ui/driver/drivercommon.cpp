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

quint32 ioctlSetServices()
{
    return FORT_IOCTL_SETSERVICES;
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

quint32 logBlockedIpHeaderSize(bool isIPv6)
{
    return FORT_LOG_BLOCKED_IP_HEADER_SIZE(isIPv6);
}

quint32 logBlockedIpSize(quint32 pathLen, bool isIPv6)
{
    return FORT_LOG_BLOCKED_IP_SIZE(pathLen, isIPv6);
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

void logBlockedIpHeaderWrite(char *output, int isIPv6, int inbound, int inherited,
        quint8 blockReason, quint8 ipProto, quint16 localPort, quint16 remotePort,
        const ip_addr_t *localIp, const ip_addr_t *remoteIp, quint32 pid, quint32 pathLen)
{
    fort_log_blocked_ip_header_write(output, isIPv6, inbound, inherited, blockReason, ipProto,
            localPort, remotePort, &localIp->v4, &remoteIp->v4, pid, pathLen);
}

void logBlockedIpHeaderRead(const char *input, int *isIPv6, int *inbound, int *inherited,
        quint8 *blockReason, quint8 *ipProto, quint16 *localPort, quint16 *remotePort,
        ip_addr_t *localIp, ip_addr_t *remoteIp, quint32 *pid, quint32 *pathLen)
{
    fort_log_blocked_ip_header_read(input, isIPv6, inbound, inherited, blockReason, ipProto,
            localPort, remotePort, &localIp->v4, &remoteIp->v4, pid, pathLen);
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

void logTimeWrite(char *output, int systemTimeChanged, qint64 unixTime)
{
    fort_log_time_write(output, systemTimeChanged, unixTime);
}

void logTimeRead(const char *input, int *systemTimeChanged, qint64 *unixTime)
{
    fort_log_time_read(input, systemTimeChanged, unixTime);
}

void confAppPermsMaskInit(void *drvConf)
{
    PFORT_CONF conf = (PFORT_CONF) drvConf;

    fort_conf_app_perms_mask_init(conf, conf->flags.group_bits);
}

bool confIpInRange(
        const void *drvConf, const quint32 *ip, bool isIPv6, bool included, int addrGroupIndex)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const PFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addrGroupIndex);

    const bool is_empty = included ? addr_group->include_is_empty : addr_group->exclude_is_empty;
    if (is_empty)
        return false;

    const PFORT_CONF_ADDR4_LIST addr_list = included
            ? fort_conf_addr_group_include_list_ref(addr_group)
            : fort_conf_addr_group_exclude_list_ref(addr_group);

    return fort_conf_ip_inlist(ip, addr_list, isIPv6);
}

bool confIp4InRange(const void *drvConf, quint32 ip, bool included, int addrGroupIndex)
{
    return confIpInRange(drvConf, &ip, /*isIPv6=*/false, included, addrGroupIndex);
}

bool confIp6InRange(const void *drvConf, const ip6_addr_t &ip, bool included, int addrGroupIndex)
{
    return confIpInRange(drvConf, &ip.addr32[0], /*isIPv6=*/true, included, addrGroupIndex);
}

quint16 confAppFind(const void *drvConf, const QString &kernelPath)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const QString kernelPathLower = kernelPath.startsWith('\\') ? kernelPath.toLower() : kernelPath;
    const quint32 len = quint32(kernelPathLower.size()) * sizeof(WCHAR);
    const WCHAR *p = (PCWCHAR) kernelPathLower.utf16();

    const FORT_APP_DATA app_data = fort_conf_app_find(
            conf, (const PVOID) p, len, fort_conf_app_exe_find, /*exe_context=*/nullptr);

    return app_data.flags.v;
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

bool provRegister(bool bootFilter)
{
    const FORT_PROV_BOOT_CONF boot_conf = {
        .boot_filter = bootFilter,
    };

    return fort_prov_trans_register(boot_conf) == 0;
}

void provUnregister()
{
    fort_prov_trans_unregister();
}

}
