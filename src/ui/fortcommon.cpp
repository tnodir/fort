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

void FortCommon::logBlockedHeaderWrite(char *output,
                                       quint32 remoteIp, quint32 pid,
                                       quint32 pathLen)
{
    fort_log_blocked_header_write(output, remoteIp, pid, pathLen);
}

void FortCommon::logBlockedHeaderRead(const char *input,
                                      quint32 *remoteIp, quint32 *pid,
                                      quint32 *pathLen)
{
    fort_log_blocked_header_read(input, remoteIp, pid, pathLen);
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
                                       quint16 *procCount)
{
    fort_log_stat_traf_header_read(input, procCount);
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

    const UINT32 count = included ? addr_group->include_n
                                  : addr_group->exclude_n;
    const UINT32 *iprange = included
            ? fort_conf_addr_group_include_ref(addr_group)
            : fort_conf_addr_group_exclude_ref(addr_group);

    return fort_conf_ip_inrange(ip, count, iprange);
}

int FortCommon::confAppIndex(const void *drvConf,
                             const QString &kernelPath)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const QString kernelPathLower = kernelPath.toLower();
    const int len = kernelPathLower.size() * sizeof(wchar_t);
    const wchar_t *p = (const wchar_t *) kernelPathLower.utf16();

    return fort_conf_app_index(conf, len, (const char *) p);
}

quint8 FortCommon::confAppGroupIndex(const void *drvConf, int appIndex)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    return fort_conf_app_group_index(conf, appIndex);
}

bool FortCommon::confAppBlocked(const void *drvConf, int appIndex)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    return fort_conf_app_blocked(conf, appIndex);
}

quint16 FortCommon::confAppPeriodBits(const void *drvConf, int hour)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;

    return fort_conf_app_period_bits(conf, hour, nullptr);
}

void FortCommon::provUnregister()
{
    fort_prov_unregister(0);
}
