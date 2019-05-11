#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>

#define OS_TICKS_PER_SECOND 1000

class OsUtil : public QObject
{
    Q_OBJECT

public:
    explicit OsUtil(QObject *parent = nullptr);

    Q_INVOKABLE static QString pidToPath(quint32 pid, bool isKernelPath = false);

    Q_INVOKABLE static void openFolder(const QString &filePath);

    static bool createGlobalMutex(const char *name);

    static quint32 lastErrorCode();
    static QString lastErrorMessage(quint32 errorCode = lastErrorCode());

    static qint32 getTickCount();
};

#endif // OSUTIL_H
