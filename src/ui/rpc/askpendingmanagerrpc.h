#ifndef ASKPENDINGMANAGERRPC_H
#define ASKPENDINGMANAGERRPC_H

#include <stat/askpendingmanager.h>

class AskPendingManagerRpc : public AskPendingManager
{
    Q_OBJECT

public:
    explicit AskPendingManagerRpc(QObject *parent = nullptr);
};

#endif // ASKPENDINGMANAGERRPC_H
