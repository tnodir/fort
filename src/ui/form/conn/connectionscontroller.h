#ifndef CONNECTIONSCONTROLLER_H
#define CONNECTIONSCONTROLLER_H

#include <QVector>

#include <form/basecontroller.h>

class ConnectionsController : public BaseController
{
    Q_OBJECT

public:
    explicit ConnectionsController(QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0);
    void deleteConns(const QVector<qint64> &connIds);
};

#endif // CONNECTIONSCONTROLLER_H
