#include "drivercommon.h"

#include <common/fort_wildmatch.h>
#include <common/fortconf.h>
#include <common/fortconf_conn.h>
#include <common/fortconf_valid.h>
#include <common/fortioctl.h>
#include <common/fortlog.h>
#include <common/fortprov.h>
#include <util/fileutil.h>

namespace {

PCFORT_CONF_ADDR_LIST confAddrList(const void *drvConf, bool included, int addrGroupIndex)
{
    PCFORT_CONF conf = (PCFORT_CONF) drvConf;
    PCFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(conf, addrGroupIndex);

    const bool is_empty = included ? addr_group->include_is_empty : addr_group->exclude_is_empty;
    if (is_empty)
        return nullptr;

    return included ? fort_conf_addr_group_include_list_ref(addr_group)
                    : fort_conf_addr_group_exclude_list_ref(addr_group);
}

using DriverCommon::ConnFilterConf;

const ConnFilterConf *connFilterConf(void *ctx)
{
    return static_cast<const ConnFilterConf *>(ctx);
}

// The Rules must be present
FORT_CONF_RULES_RT connFilterRulesRt(const ConnFilterConf &cf)
{
    PCFORT_CONF_IO conf_io = PCFORT_CONF_IO(cf.drvConfIo);
    PCFORT_CONF_RULES rules = PCFORT_CONF_RULES(cf.drvRules);
    PCFORT_CONF_ZONES zones = PCFORT_CONF_ZONES(cf.drvZones);

    const UINT64 active_periods_mask = conf_io ? conf_io->periods.active_mask : 0;

    return fort_conf_rules_rt_make(rules, zones, active_periods_mask);
}

BOOL connZonesIpIncluded(void *ctx, PCFORT_CONF_META_CONN conn, UCHAR *zone_id, UINT32 zones_mask)
{
    PCFORT_CONF_ZONES zones = PCFORT_CONF_ZONES(connFilterConf(ctx)->drvZones);
    if (zones == nullptr)
        return false;

    return fort_conf_zones_ip_included(zones, conn, zone_id, zones_mask);
}

BOOL connZonesConnFiltered(
        void *ctx, PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt)
{
    PCFORT_CONF_ZONES zones = PCFORT_CONF_ZONES(connFilterConf(ctx)->drvZones);
    if (zones == nullptr)
        return false;

    return fort_conf_zones_conn_filtered(zones, conn, opt, /*fast_check=*/TRUE);
}

BOOL connRulesConnFiltered(void *ctx, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    const ConnFilterConf *cf = connFilterConf(ctx);
    if (cf->drvRules == nullptr)
        return false;

    const FORT_CONF_RULES_RT rules_rt = connFilterRulesRt(*cf);

    return fort_conf_rules_rt_conn_filtered(&rules_rt, conn, rule_id);
}

UINT16 connRulesGlobConnFiltered(void *ctx, PFORT_CONF_META_CONN conn, BOOL is_post)
{
    const ConnFilterConf *cf = connFilterConf(ctx);
    if (cf->drvRules == nullptr)
        return 0;

    const FORT_CONF_RULES_RT rules_rt = connFilterRulesRt(*cf);

    return fort_conf_rules_glob_conn_filtered(&rules_rt, conn, is_post);
}

BOOL connGroupsMaskBlocked(void *ctx, UINT32 groups_mask)
{
    return DriverCommon::confGroupsMaskBlocked(connFilterConf(ctx)->drvGroups, groups_mask);
}

UINT16 connGroupsRulesConnFiltered(void *ctx, PFORT_CONF_META_CONN conn, UINT32 groups_mask)
{
    return DriverCommon::confGroupsRulesConnFiltered(*connFilterConf(ctx), conn, groups_mask);
}

inline constexpr FORT_CONF_CONN_FILTER_FUNCS connFilterFuncs = {
    .zones_ip_included = &connZonesIpIncluded,
    .zones_conn_filtered = &connZonesConnFiltered,
    .rules_conn_filtered = &connRulesConnFiltered,
    .rules_glob_conn_filtered = &connRulesGlobConnFiltered,
    .groups_mask_blocked = &connGroupsMaskBlocked,
    .groups_rules_conn_filtered = &connGroupsRulesConnFiltered,
};

FORT_CONF_META_CONN connFilterInput(PCFORT_CONF_META_CONN conn)
{
    return {
        .inbound = conn->inbound,
        .isIPv6 = conn->isIPv6,
        .profile_id = conn->profile_id,
        .is_loopback = conn->is_loopback,
        .ip_proto = conn->ip_proto,
        .local_port = conn->local_port,
        .remote_port = conn->remote_port,
        .local_ip = conn->local_ip,
        .remote_ip = conn->remote_ip,
    };
}

DriverCommon::ConnFilterResult connFilterResult(PCFORT_CONF_META_CONN conn)
{
    if (conn->ignore)
        return DriverCommon::ConnFilterIgnored;

    if (conn->ask_to_connect)
        return DriverCommon::ConnFilterAsk;

    return conn->act.blocked ? DriverCommon::ConnFilterBlocked : DriverCommon::ConnFilterAllowed;
}

}

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

quint32 ioctlSetGroups()
{
    return FORT_IOCTL_SETGROUPS;
}

quint32 ioctlSetGroupFlags()
{
    return FORT_IOCTL_SETGROUPFLAGS;
}

quint32 ioctlSetSpeedLimits()
{
    return FORT_IOCTL_SETSPEEDLIMITS;
}

quint32 ioctlSetSpeedLimitFlags()
{
    return FORT_IOCTL_SETSPEEDLIMITFLAGS;
}

quint32 ioctlGetSpeedLimitStatus()
{
    return FORT_IOCTL_GETSPEEDLIMITSTATUS;
}

quint32 ioctlSetPeriods()
{
    return FORT_IOCTL_SETPERIODS;
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

int speedLimitsStatusSize()
{
    return sizeof(FORT_SPEED_LIMITS_STATUS);
}

SpeedLimitStatus speedLimitStatus(const QByteArray &data, quint8 limitId)
{
    SpeedLimitStatus status;

    if (data.size() < speedLimitsStatusSize())
        return status;

    const int index = limitId - 1;
    if (index < 0 || index >= FORT_CONF_SPEED_LIMIT_MAX)
        return status;

    PCFORT_SPEED_LIMITS_STATUS limitsStatus = PCFORT_SPEED_LIMITS_STATUS(data.constData());
    if ((limitsStatus->mask & (1u << index)) == 0)
        return status;

    PCFORT_SPEED_LIMIT_STATUS limitStatus = &limitsStatus->limits[index];

    status.isValid = true;
    status.queuedBytes = limitStatus->queued_bytes;
    status.droppedCount = limitStatus->dropped_count;
    status.lostCount = limitStatus->lost_count;

    return status;
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

quint32 logConnInheritPathOffset(quint16 pathLen, bool isIPv6)
{
    return FORT_LOG_CONN_INHERIT_PATH_OFFSET(pathLen, isIPv6);
}

quint32 logConnSize(quint16 pathLen, quint16 inheritPathLen, bool isIPv6)
{
    return FORT_LOG_CONN_SIZE(pathLen, inheritPathLen, isIPv6);
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

void logConnHeaderWrite(
        char *output, PCFORT_CONF_META_CONN conn, quint16 pathLen, quint16 inheritPathLen)
{
    fort_log_conn_header_write(output, conn, pathLen, inheritPathLen);
}

void logConnHeaderRead(
        const char *input, PFORT_CONF_META_CONN conn, quint16 *pathLen, quint16 *inheritPathLen)
{
    fort_log_conn_header_read(input, conn, pathLen, inheritPathLen);
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

bool confIp4InRange(const void *drvConf, quint32 ip, bool included, int addrGroupIndex)
{
    PCFORT_CONF_ADDR_LIST addr_list = confAddrList(drvConf, included, addrGroupIndex);
    if (addr_list == nullptr)
        return false;

    const ip_addr_t ip_addr = { .v4 = ip };

    return fort_conf_ip_inlist(addr_list, ip_addr, /*isIPv6=*/false);
}

bool confIp6InRange(const void *drvConf, const ip6_addr_t ip, bool included, int addrGroupIndex)
{
    PCFORT_CONF_ADDR_LIST addr_list = confAddrList(drvConf, included, addrGroupIndex);
    if (addr_list == nullptr)
        return false;

    const ip_addr_t ip_addr = { .v6 = ip };

    return fort_conf_ip_inlist(addr_list, ip_addr, /*isIPv6=*/true);
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

bool confRulesConnFiltered(
        const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId, const void *drvZones)
{
    if (drvRules == nullptr)
        return false;

    PCFORT_CONF_RULES rules = PCFORT_CONF_RULES(drvRules);
    PCFORT_CONF_ZONES zones = PCFORT_CONF_ZONES(drvZones);

    const FORT_CONF_RULES_RT rules_rt =
            fort_conf_rules_rt_make(rules, zones, /*active_periods_mask=*/0);

    return fort_conf_rules_rt_conn_filtered(&rules_rt, conn, ruleId);
}

bool confGroupsMaskBlocked(const void *drvGroups, quint32 groupsMask)
{
    if (drvGroups == nullptr)
        return false;

    PCFORT_CONF_GROUPS groups = static_cast<PCFORT_CONF_GROUPS>(drvGroups);

    return fort_conf_groups_mask_blocked(groups, groupsMask);
}

quint16 confGroupsRulesConnFiltered(
        const ConnFilterConf &cf, PFORT_CONF_META_CONN conn, quint32 groupsMask)
{
    PCFORT_CONF_GROUPS groups = static_cast<PCFORT_CONF_GROUPS>(cf.drvGroups);
    if (groups == nullptr || cf.drvRules == nullptr)
        return 0;

    const FORT_CONF_RULES_RT rules_rt = connFilterRulesRt(cf);

    return fort_conf_groups_rules_conn_filtered(groups, &rules_rt, conn, groupsMask);
}

bool confRulesConnBlocked(const void *drvRules, PFORT_CONF_META_CONN conn, quint16 ruleId)
{
    conn->act.blocked = TRUE; /* default block */

    return confRulesConnFiltered(drvRules, conn, ruleId) && conn->act.blocked;
}

ConnFilterResult confConnFilter(
        const ConnFilterConf &cf, PFORT_CONF_META_CONN conn, const FORT_APP_DATA &appData)
{
    *conn = connFilterInput(conn); // reset the results

    PCFORT_CONF conf = &PCFORT_CONF_IO(cf.drvConfIo)->conf;

    if (fort_conf_conn_local_allowed(conn, conf->flags))
        return ConnFilterAllowed;

    const FORT_CONF_CONN_FILTER filter = {
        .conf_flags = conf->flags,
        .conf = conf,
        .funcs = &connFilterFuncs,
        .ctx = const_cast<ConnFilterConf *>(&cf),
    };

    if (!fort_conf_conn_flags_filtered(&filter, conn)) {
        fort_conf_conn_app_allowed(&filter, conn, appData);
    }

    return connFilterResult(conn);
}

bool confIoValid(const void *drvConfIo, quint32 len)
{
    return fort_conf_io_valid(PCFORT_CONF_IO(drvConfIo), len);
}

bool confZonesValid(const void *drvZones, quint32 len)
{
    return fort_conf_zones_valid(PCFORT_CONF_ZONES(drvZones), len);
}

bool confRulesValid(const void *drvRules, quint32 len)
{
    return fort_conf_rules_valid(PCFORT_CONF_RULES(drvRules), len);
}

bool provRegister(bool bootFilter)
{
    const FORT_PROV_INIT_CONF init_conf = {
        .sublayer_weight = FORT_SUBLAYER_MAX_WEIGHT,
    };

    fort_prov_init(init_conf);

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
