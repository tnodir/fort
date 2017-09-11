#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>
#include <QVariant>

class OsUtil : public QObject
{
    Q_OBJECT

public:
    explicit OsUtil(QObject *parent = nullptr);

    Q_INVOKABLE static void setClipboardData(const QVariant &data);

    Q_INVOKABLE static QString pidToDosPath(quint32 pid);

    static bool createGlobalMutex(const char *name);
};

#endif // OSUTIL_H
