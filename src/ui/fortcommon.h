#ifndef FORTCOMMON_H
#define FORTCOMMON_H

#include <QObject>
#include <QString>

class FortCommon : public QObject
{
    Q_OBJECT

public:
    explicit FortCommon(QObject *parent = nullptr);

    static QString deviceName();

    static quint32 ioctlValidate();
    static quint32 ioctlSetConf();
    static quint32 ioctlSetFlags();
    static quint32 ioctlGetLog();

    static int bufferSize();

    static quint32 confIoConfOff();

    static quint32 logBlockedHeaderSize();
    static quint32 logBlockedSize(quint32 pathLen);

    static quint32 logProcNewHeaderSize();
    static quint32 logProcNewSize(quint32 pathLen);

    static quint32 logStatHeaderSize();
    static quint32 logStatTrafSize(quint16 procCount);
    static quint32 logStatSize(quint16 procCount);

    static quint32 logType(const char *input);

    static void logBlockedHeaderWrite(char *output,
                                      quint32 remoteIp, quint32 pid,
                                      quint32 pathLen);
    static void logBlockedHeaderRead(const char *input,
                                     quint32 *remoteIp, quint32 *pid,
                                     quint32 *pathLen);

    static void logProcNewHeaderWrite(char *output,
                                      quint32 pid, quint32 pathLen);
    static void logProcNewHeaderRead(const char *input,
                                     quint32 *pid, quint32 *pathLen);

    static void logStatTrafHeaderRead(const char *input,
                                      quint16 *procCount);

    static void confAppPermsMaskInit(void *drvConf);
    static bool confIpInRange(const void *drvConf, quint32 ip,
                              bool included = false, int addrGroupIndex = 0);
    static int confAppIndex(const void *drvConf,
                            const QString &kernelPath);
    static quint8 confAppGroupIndex(const void *drvConf, int appIndex);
    static bool confAppBlocked(const void *drvConf, int appIndex);
    static quint16 confAppPeriodBits(const void *drvConf,
                                     quint8 hour, quint8 minute);

    static bool isTimeInPeriod(quint8 hour, quint8 minute,
                               quint8 fromHour, quint8 fromMinute,
                               quint8 toHour, quint8 toMinute);

    static void provUnregister();
};

#endif // FORTCOMMON_H
