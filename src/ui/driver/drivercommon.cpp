#include "drivercommon.h"

#include <common/fort_wildmatch.h>
#include <common/fortconf.h>
#include <common/fortioctl.h>
#include <common/fortlog.h>
#include <common/fortprov.h>
#include <util/fileutil.h>

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

quint32 ioctlSetRules()
{
    return FORT_IOCTL_SETRULES;
}

quint32 ioctlSetRuleFlag()
{
    return FORT_IOCTL_SETRULEFLAG;
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

quint32 logAppHeaderSize()
{
    return FORT_LOG_APP_HEADER_SIZE;
}

quint32 logAppSize(quint16 pathLen)
{
    return FORT_LOG_APP_SIZE(pathLen);
}

quint32 logConnHeaderSize(bool isIPv6)
{
    return FORT_LOG_CONN_HEADER_SIZE(isIPv6);
}

quint32 logConnSize(quint16 pathLen, bool isIPv6)
{
    return FORT_LOG_CONN_SIZE(pathLen, isIPv6);
}

quint32 logProcNewHeaderSize()
{
    return FORT_LOG_PROC_NEW_HEADER_SIZE;
}

quint32 logProcNewSize(quint16 pathLen)
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

quint32 logProcKillSize()
{
    return FORT_LOG_PROC_KILL_SIZE;
}

quint8 logType(const char *input)
{
    return fort_log_type(input);
}

void logAppHeaderWrite(char *output, bool blocked, quint32 pid, quint16 pathLen)
{
    fort_log_app_header_write(output, blocked, pid, pathLen);
}

void logAppHeaderRead(const char *input, int *blocked, quint32 *pid, quint16 *pathLen)
{
    fort_log_app_header_read(input, blocked, pid, pathLen);
}

void logConnHeaderWrite(char *output, PCFORT_CONF_META_CONN conn, quint16 pathLen)
{
    fort_log_conn_header_write(output, conn, pathLen);
}

void logConnHeaderRead(const char *input, PFORT_CONF_META_CONN conn, quint16 *pathLen)
{
    fort_log_conn_header_read(input, conn, pathLen);
}

void logProcNewHeaderWrite(char *output, quint32 appId, quint32 pid, quint16 pathLen)
{
    fort_log_proc_new_header_write(output, appId, pid, pathLen);
}

void logProcNewHeaderRead(const char *input, quint32 *appId, quint32 *pid, quint16 *pathLen)
{
    fort_log_proc_new_header_read(input, appId, pid, pathLen);
}

void logStatTrafHeaderRead(const char *input, quint16 *procCount)
{
    fort_log_stat_traf_header_read(input, procCount);
}

void logTimeWrite(char *output, qint64 unixTime, int systemTimeChanged)
{
    fort_log_time_write(output, unixTime, systemTimeChanged);
}

void logTimeRead(const char *input, qint64 *unixTime, int *systemTimeChanged)
{
    fort_log_time_read(input, unixTime, systemTimeChanged);
}

void logProcKillWrite(char *output, quint32 pid)
{
    fort_log_proc_kill_write(output, pid);
}

void logProcKillRead(const char *input, quint32 *pid)
{
    fort_log_proc_kill_read(input, pid);
}

bool confIpInRange(
        const void *drvConf, const ip_addr_t ip, bool isIPv6, bool included, int addrGroupIndex)
{
    PCFORT_CONF conf = (PCFORT_CONF) drvConf;
    PCFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addrGroupIndex);

    const bool is_empty = included ? addr_group->include_is_empty : addr_group->exclude_is_empty;
    if (is_empty)
        return false;

    const PFORT_CONF_ADDR_LIST addr_list = included
            ? fort_conf_addr_group_include_list_ref(addr_group)
            : fort_conf_addr_group_exclude_list_ref(addr_group);

    return fort_conf_ip_inlist(addr_list, ip, isIPv6);
}

bool confIp4InRange(const void *drvConf, quint32 ip, bool included, int addrGroupIndex)
{
    const ip_addr_t ip_addr = { .v4 = ip };

    return confIpInRange(drvConf, ip_addr, /*isIPv6=*/false, included, addrGroupIndex);
}

bool confIp6InRange(const void *drvConf, const ip6_addr_t ip, bool included, int addrGroupIndex)
{
    const ip_addr_t ip_addr = { .v6 = ip };

    return confIpInRange(drvConf, ip_addr, /*isIPv6=*/true, included, addrGroupIndex);
}

FORT_APP_DATA confAppFind(const void *drvConf, const QString &appPath)
{
    PCFORT_CONF conf = PCFORT_CONF(drvConf);
    const QString normPath = FileUtil::normalizePath(appPath);

    const FORT_APP_PATH path = {
        .len = quint16(normPath.size() * sizeof(WCHAR)),
        .buffer = normPath.utf16(),
    };

    const FORT_APP_DATA app_data =
            fort_conf_app_find(conf, &path, fort_conf_app_exe_find, /*exe_context=*/nullptr);

    return app_data;
}

bool wildMatch(const QString &pattern, const QString &text)
{
    return wildmatch((const wm_char *) pattern.utf16(), (const wm_char *) text.utf16()) == WM_MATCH;
}

bool wildMatchPath(const QString &pattern, const QString &path)
{
    const auto normPattern = FileUtil::normalizePath(pattern);
    const auto normPath = FileUtil::normalizePath(path);

    return wildMatch(normPattern, normPath);
}

bool confRulesConnFiltered(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId)
{
    PCFORT_CONF_RULES rules = PCFORT_CONF_RULES(drvRules);

    return fort_conf_rules_conn_filtered(rules, /*zones=*/nullptr, conn, ruleId);
}

bool confRulesConnBlocked(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId)
{
    conn->blocked = TRUE; /* default block */

    return confRulesConnFiltered(drvRules, conn, ruleId) && conn->blocked;
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
