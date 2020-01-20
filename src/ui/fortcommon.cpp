#include "fortcommon.h"

#define _WIN32_WINNT    0x0601
#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>
#include <winioctl.h>
#include <fwpmu.h>

#include "../common/common.h"
#include "../common/fortconf.h"

#include "../common/fortconf.c"
#include "../common/fortlog.c"
#include "../common/fortprov.c"

FortCommon::FortCommon(QObject *parent) :
    QObject(parent)
{
}

QString FortCommon::deviceName()
{
    return QLatin1String(FORT_DEVICE_NAME);
}

quint32 FortCommon::ioctlValidate()
{
    return FORT_IOCTL_VALIDATE;
}

quint32 FortCommon::ioctlSetConf()
{
    return FORT_IOCTL_SETCONF;
}

quint32 FortCommon::ioctlSetFlags()
{
    return FORT_IOCTL_SETFLAGS;
}

quint32 FortCommon::ioctlGetLog()
{
    return FORT_IOCTL_GETLOG;
}

quint32 FortCommon::ioctlAddApp()
{
    return FORT_IOCTL_ADDAPP;
}

quint32 FortCommon::ioctlDelApp()
{
    return FORT_IOCTL_DELAPP;
}

quint32 FortCommon::userErrorCode()
{
    return FORT_ERROR_USER_ERROR;
}

qint64 FortCommon::systemToUnixTime(qint64 systemTime)
{
    return fort_system_to_unix_time(systemTime);
}

int FortCommon::bufferSize()
{
    return FORT_BUFFER_SIZE;
}

quint32 FortCommon::confIoConfOff()
{
    return FORT_CONF_IO_CONF_OFF;
}

quint32 FortCommon::logBlockedHeaderSize()
{
    return FORT_LOG_BLOCKED_HEADER_SIZE;
}

quint32 FortCommon::logBlockedSize(quint32 pathLen)
{
    return FORT_LOG_BLOCKED_SIZE(pathLen);
}

quint32 FortCommon::logProcNewHeaderSize()
{
    return FORT_LOG_PROC_NEW_HEADER_SIZE;
}

quint32 FortCommon::logProcNewSize(quint32 pathLen)
{
    return FORT_LOG_PROC_NEW_SIZE(pathLen);
}

quint32 FortCommon::logStatHeaderSize()
{
    return FORT_LOG_STAT_HEADER_SIZE;
}

quint32 FortCommon::logStatTrafSize(quint16 procCount)
{
    return FORT_LOG_STAT_TRAF_SIZE(procCount);
}

quint32 FortCommon::logStatSize(quint16 procCount)
{
    return FORT_LOG_STAT_SIZE(procCount);
}

quint32 FortCommon::logType(const char *input)
{
    return fort_log_type(input);
}

void FortCommon::logBlockedHeaderWrite(char *output, bool blocked,
                                       quint32 remoteIp, quint16 remotePort,
                                       quint8 ipProto, quint32 pid, quint32 pathLen)
{
    fort_log_blocked_header_write(output, blocked, remoteIp, remotePort,
                                  ipProto, pid, pathLen);
}

void FortCommon::logBlockedHeaderRead(const char *input, int *blocked,
                                      quint32 *remoteIp, quint16 *remotePort,
                                      quint8 *ipProto, quint32 *pid, quint32 *pathLen)
{
    fort_log_blocked_header_read(input, blocked, remoteIp, remotePort,
                                 ipProto, pid, pathLen);
}

void FortCommon::logProcNewHeaderWrite(char *output,
                                       quint32 pid, quint32 pathLen)
{
    fort_log_proc_new_header_write(output, pid, pathLen);
}

void FortCommon::logProcNewHeaderRead(const char *input,
                                      quint32 *pid, quint32 *pathLen)
{
    fort_log_proc_new_header_read(input, pid, pathLen);
}

void FortCommon::logStatTrafHeaderRead(const char *input,
                                       qint64 *unixTime,
                                       quint16 *procCount)
{
    fort_log_stat_traf_header_read(input, unixTime, procCount);
}

void FortCommon::confAppPermsMaskInit(void *drvConf)
{
    PFORT_CONF conf = (PFORT_CONF) drvConf;

    fort_conf_app_perms_mask_init(conf, conf->flags.group_bits);
}

bool FortCommon::confIpInRange(const void *drvConf, quint32 ip,
                               bool included, int addrGroupIndex)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const PFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(
      conf, addrGroupIndex);

    const UINT32 ip_count = included ? addr_group->include_ip_n
                                     : addr_group->exclude_ip_n;

    const UINT32 *iparr = included
            ? fort_conf_addr_group_include_ip_ref(addr_group)
            : fort_conf_addr_group_exclude_ip_ref(addr_group);

    if (fort_conf_ip_inarr(ip, ip_count, iparr))
        return true;

    const UINT32 pair_count = included ? addr_group->include_pair_n
                                       : addr_group->exclude_pair_n;

    const UINT32 *iprange = included
            ? fort_conf_addr_group_include_pair_ref(addr_group)
            : fort_conf_addr_group_exclude_pair_ref(addr_group);

    return fort_conf_ip_inrange(ip, pair_count, iprange);
}

quint16 FortCommon::confAppFind(const void *drvConf,
                                const QString &kernelPath)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const QString kernelPathLower = kernelPath.toLower();
    const quint32 len = quint32(kernelPathLower.size()) * sizeof(wchar_t);
    const wchar_t *p = (const wchar_t *) kernelPathLower.utf16();

    const FORT_APP_FLAGS app_flags =
            fort_conf_app_find(conf, (const char *) p, len,
                               fort_conf_app_exe_find);

    return app_flags.v;
}

quint8 FortCommon::confAppGroupIndex(quint16 appFlags)
{
    const FORT_APP_FLAGS app_flags = {appFlags};

    return app_flags.group_index;
}

bool FortCommon::confAppBlocked(const void *drvConf, quint16 appFlags)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    return fort_conf_app_blocked(conf, {appFlags});
}

quint16 FortCommon::confAppPeriodBits(const void *drvConf,
                                      quint8 hour, quint8 minute)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    FORT_TIME time;
    time.hour = hour;
    time.minute = minute;

    return fort_conf_app_period_bits(conf, time, nullptr);
}

bool FortCommon::isTimeInPeriod(quint8 hour, quint8 minute,
                                quint8 fromHour, quint8 fromMinute,
                                quint8 toHour, quint8 toMinute)
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

void FortCommon::provUnregister()
{
    fort_prov_unregister(nullptr);
}
