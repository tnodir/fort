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

    static quint32 ioctlSetConf();
    static quint32 ioctlSetFlags();
    static quint32 ioctlGetLog();

    static int bufferSize();

    static quint32 logBlockedHeaderSize();
    static quint32 logBlockedSize(quint32 pathLen);

    static quint32 logProcNewHeaderSize();
    static quint32 logProcNewSize(quint32 pathLen);

    static quint32 logStatHeaderSize();
    static quint32 logStatProcSize(quint16 procCount);
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
                              bool included = false);
    static int confAppIndex(const void *drvConf,
                            const QString &kernelPath);
    static quint8 confAppGroupIndex(const void *drvConf, int appIndex);
    static bool confAppBlocked(const void *drvConf, int appIndex);

    static void provUnregister();
};

#endif // FORTCOMMON_H
