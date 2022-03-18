#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>

enum BeepType {
    BeepError = 0x10,
    BeepQuestion = 0x20,
    BeepWarning = 0x30,
    BeepInfo = 0x40,
    BeepSimple = -1
};

#define OS_TICKS_PER_SECOND 1000

class OsUtil
{
public:
    static QString pidToPath(quint32 pid, bool isKernelPath = false);

    static bool openFolder(const QString &filePath);
    static bool openUrlOrFolder(const QString &path);

    static void *createMutex(const char *name, bool &isSingleInstance);
    static void closeMutex(void *mutexHandle);

    static quint32 lastErrorCode();
    static QString errorMessage(quint32 errorCode = lastErrorCode());

    static qint32 getTickCount();

    static QString userName();
    static bool isUserAdmin();

    static bool beep(BeepType type = BeepSimple);

    static void showConsole(bool visible);
    static void writeToConsole(const char *category, const QString &message);

    static void setThreadIsBusy(bool on);
};

#endif // OSUTIL_H
