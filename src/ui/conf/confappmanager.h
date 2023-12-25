#ifndef CONFAPPMANAGER_H
#define CONFAPPMANAGER_H

#include <QObject>

class ConfAppManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfAppManager(QObject *parent = nullptr);

signals:
};

#endif // CONFAPPMANAGER_H
