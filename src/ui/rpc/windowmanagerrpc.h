#ifndef WINDOWMANAGERRPC_H
#define WINDOWMANAGERRPC_H

#include "../manager/windowmanager.h"

class WindowManagerRpc : public WindowManager
{
    Q_OBJECT

public:
    explicit WindowManagerRpc(QObject *parent = nullptr);

    void setUp() override { }
};

#endif // WINDOWMANAGERRPC_H
