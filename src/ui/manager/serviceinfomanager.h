#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QObject>

class ServiceInfoManager : public QObject
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);
};

#endif // SERVICEINFOMANAGER_H
