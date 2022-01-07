#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QHash>
#include <QObject>

#include <util/ioc/iocservice.h>

#include "serviceinfo.h"

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);

    int groupIndexByName(const QString &name) const;

    static QVector<ServiceInfo> loadServiceInfoList();

signals:
    void serviceChanged(quint32 processId, int groupIndex = -1);

private:
    QHash<QString, int> m_serviceGroups;
};

#endif // SERVICEINFOMANAGER_H
