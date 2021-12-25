#ifndef LOGMANAGERRPC_H
#define LOGMANAGERRPC_H

#include <log/logmanager.h>

class LogManagerRpc : public LogManager
{
    Q_OBJECT

public:
    explicit LogManagerRpc(QObject *parent = nullptr);

    void setActive(bool /*active*/) override { }

    void setUp() override { }
    void tearDown() override { }
};

#endif // LOGMANAGERRPC_H
