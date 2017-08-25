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

