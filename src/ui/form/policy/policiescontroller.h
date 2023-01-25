#ifndef POLICIESCONTROLLER_H
#define POLICIESCONTROLLER_H

#include <form/basecontroller.h>

class PoliciesController : public BaseController
{
    Q_OBJECT

public:
    explicit PoliciesController(QObject *parent = nullptr);

    void initialize();
};

#endif // POLICIESCONTROLLER_H
