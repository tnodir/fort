#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

class FortManager : public QObject
{
    Q_OBJECT

public:
    explicit FortManager(QObject *parent = nullptr);

signals:

public slots:
};

#endif // FORTMANAGER_H
