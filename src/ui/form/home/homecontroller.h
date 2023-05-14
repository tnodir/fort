#ifndef HOMECONTROLLER_H
#define HOMECONTROLLER_H

#include <form/basecontroller.h>

class HomeController : public BaseController
{
    Q_OBJECT

public:
    explicit HomeController(QObject *parent = nullptr);

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);
};

#endif // HOMECONTROLLER_H
