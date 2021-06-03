#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QObject>

#include "../util/ioc/iocservice.h"

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);
};

#endif // SERVICEINFOMANAGER_H
