#include "fortcommon.h"

#define _WIN32_WINNT	0x0600

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

quint32 FortCommon::bufferSize()
{
    return FORT_BUFFER_SIZE;
}

quint32 FortCommon::logSize(quint32 pathLen)
{
    return FORT_LOG_SIZE(pathLen);
}

quint32 FortCommon::logHeaderSize()
{
    return FORT_LOG_HEADER_SIZE;
}

void FortCommon::logHeaderWrite(char *output,
                                quint32 remoteIp, quint32 pid,
                                quint32 pathLen)
{
    fort_log_header_write(output, remoteIp, pid, pathLen);
}

void FortCommon::logHeaderRead(const char *input,
                               quint32 *remoteIp, quint32 *pid,
                               quint32 *pathLen)
{
    fort_log_header_read(input, remoteIp, pid, pathLen);
}

void FortCommon::confAppPermsMaskInit(void *drvConf)
{
    fort_conf_app_perms_mask_init((PFORT_CONF) drvConf);
}

bool FortCommon::confIpInRange(const void *drvConf, quint32 ip,
                               bool included)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const char *data = (const char *) conf + conf->data_off;

    const quint32 count = included ? conf->ip_include_n : conf->ip_exclude_n;
    const quint32 fromOff = included ? conf->ip_from_include_off : conf->ip_from_exclude_off;
    const quint32 toOff = included ? conf->ip_to_include_off : conf->ip_to_exclude_off;

    return fort_conf_ip_inrange(ip, count,
                                (const quint32 *) (data + fromOff),
                                (const quint32 *) (data + toOff));
}

bool FortCommon::confAppBlocked(const void *drvConf,
                                const QString &dosPath, bool *notify)
{
    const PFORT_CONF conf = (const PFORT_CONF) drvConf;
    const QString dosPathLower = dosPath.toLower();
    const int len = dosPathLower.size() * sizeof(wchar_t);
    const wchar_t *p = (const wchar_t *) dosPathLower.utf16();
    BOOL blocked, notifyUser;

    blocked = fort_conf_app_blocked(conf, len, (const char *) p, &notifyUser);

    if (notify) {
        *notify = notifyUser;
    }

    return blocked;
}

uint FortCommon::provRegister(bool boot)
{
    return fort_prov_register(TRUE, boot, NULL, NULL);
}

void FortCommon::provUnregister()
{
    fort_prov_unregister();
}
