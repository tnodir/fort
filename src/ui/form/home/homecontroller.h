#ifndef HOMECONTROLLER_H
#define HOMECONTROLLER_H

#include <form/basecontroller.h>

class HomeController : public BaseController
{
    Q_OBJECT

public:
    explicit HomeController(QObject *parent = nullptr);
};

#endif // HOMECONTROLLER_H
