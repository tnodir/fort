#ifndef CONNECTIONSCONTROLLER_H
#define CONNECTIONSCONTROLLER_H

#include <form/basecontroller.h>

class ConnectionsController : public BaseController
{
    Q_OBJECT

public:
    explicit ConnectionsController(QObject *parent = nullptr);

    void deleteConn(qint64 connIdTo = 0);
};

#endif // CONNECTIONSCONTROLLER_H
