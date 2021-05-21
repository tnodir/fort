#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include <QObject>

class ServiceListModel : public QObject
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject *parent = nullptr);
};

#endif // SERVICELISTMODEL_H
