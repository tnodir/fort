#ifndef HOSTINFO_H
#define HOSTINFO_H

#include <QObject>

class HostInfo
{
    Q_GADGET
    Q_PROPERTY(QString hostName MEMBER hostName CONSTANT)

public:
    QString hostName;
};

#endif // HOSTINFO_H
