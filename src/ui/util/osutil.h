#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>

class OsUtil : public QObject
{
    Q_OBJECT

public:
    explicit OsUtil(QObject *parent = nullptr);

    Q_INVOKABLE static QString pidToPath(quint32 pid, bool isKernelPath = false);

    static bool createGlobalMutex(const char *name);

    static quint32 lastErrorCode();
    static QString lastErrorMessage(quint32 errorCode = lastErrorCode());
};

#endif // OSUTIL_H
