#ifndef FORTCOMMON_H
#define FORTCOMMON_H

#include <QObject>

class FortCommon : public QObject
{
    Q_OBJECT

public:
    explicit FortCommon(QObject *parent = nullptr);

    static QString deviceName();

    static quint32 ioctlSetConf();
    static quint32 ioctlSetFlags();
    static quint32 ioctlGetLog();

    static quint32 bufferSize();
    static quint32 logSize(quint32 pathLen);
    static quint32 logHeaderSize();

    static void logHeaderWrite(char *output,
                               quint32 remoteIp, quint32 pid,
                               quint32 pathLen);
    static void logHeaderRead(const char *input,
                              quint32 *remoteIp, quint32 *pid,
                              quint32 *pathLen);

    static void confAppPermsMaskInit(void *drvConf);
    static bool confIpInRange(const void *drvConf, quint32 ip,
                              bool included = false);
    static bool confAppBlocked(const void *drvConf,
                               const QString &dosPath, bool *notify = 0);

    static uint provRegister(bool boot);
    static void provUnregister();
};

#endif // FORTCOMMON_H
