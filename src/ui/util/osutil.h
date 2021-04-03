#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>

#define OS_TICKS_PER_SECOND 1000

class OsUtil
{
public:
    static QString pidToPath(quint32 pid, bool isKernelPath = false);

    static bool openUrlExternally(const QUrl &url);
    static bool openFolder(const QString &filePath);
    static bool openUrlOrFolder(const QString &path);

    static void *createMutex(const char *name, bool &isSingleInstance);
    static void closeMutex(void *mutexHandle);

    static quint32 lastErrorCode();
    static QString errorMessage(quint32 errorCode = lastErrorCode());

    static qint32 getTickCount();

    static bool isUserAdmin();
};

#endif // OSUTIL_H
