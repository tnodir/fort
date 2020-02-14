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
    static quint32 ioctlAddApp();
    static quint32 ioctlDelApp();

    static quint32 userErrorCode();

    static qint64 systemToUnixTime(qint64 systemTime);

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

    static void logBlockedHeaderWrite(char *output, bool blocked,
                                      quint32 remoteIp, quint16 remotePort,
                                      quint8 ipProto, quint32 pid, quint32 pathLen);
    static void logBlockedHeaderRead(const char *input, int *blocked,
                                     quint32 *remoteIp, quint16 *remotePort,
                                     quint8 *ipProto, quint32 *pid, quint32 *pathLen);

    static void logProcNewHeaderWrite(char *output,
                                      quint32 pid, quint32 pathLen);
    static void logProcNewHeaderRead(const char *input,
                                     quint32 *pid, quint32 *pathLen);

    static void logStatTrafHeaderRead(const char *input,
                                      qint64 *unixTime,
                                      quint16 *procCount);

    static void confAppPermsMaskInit(void *drvConf);
    static bool confIpInRange(const void *drvConf, quint32 ip,
                              bool included = false, int addrGroupIndex = 0);
    static quint16 confAppFind(const void *drvConf,
                               const QString &kernelPath);
    static quint8 confAppGroupIndex(quint16 appFlags);
    static bool confAppBlocked(const void *drvConf, quint16 appFlags);
    static quint16 confAppPeriodBits(const void *drvConf,
                                     quint8 hour, quint8 minute);

    static bool isTimeInPeriod(quint8 hour, quint8 minute,
                               quint8 fromHour, quint8 fromMinute,
                               quint8 toHour, quint8 toMinute);

    static int bitScanForward(quint32 mask);

    static void provUnregister();
};

#endif // FORTCOMMON_H
