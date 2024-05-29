#ifndef DBERRORMANAGERRPC_H
#define DBERRORMANAGERRPC_H

#include <manager/dberrormanager.h>

class DbErrorManagerRpc : public DbErrorManager
{
    Q_OBJECT

public:
    explicit DbErrorManagerRpc(QObject *parent = nullptr);

protected:
    void setupTimer() override { }
};

#endif // DBERRORMANAGERRPC_H
