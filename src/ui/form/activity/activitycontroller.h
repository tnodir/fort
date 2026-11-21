#ifndef ACTIVITYCONTROLLER_H
#define ACTIVITYCONTROLLER_H

#include <form/basecontroller.h>

class ActivityController : public BaseController
{
    Q_OBJECT

public:
    explicit ActivityController(QObject *parent = nullptr);
};

#endif // ACTIVITYCONTROLLER_H
