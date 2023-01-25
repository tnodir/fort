#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <form/basecontroller.h>

class TrayController : public BaseController
{
    Q_OBJECT

public:
    explicit TrayController(QObject *parent = nullptr);
};

#endif // TRAYCONTROLLER_H
