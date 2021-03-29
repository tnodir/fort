#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>

class ServiceManager : public QObject
{
    Q_OBJECT

public:
    explicit ServiceManager(QObject *parent = nullptr);

    static bool isServiceInstalled();
};

#endif // SERVICEMANAGER_H
