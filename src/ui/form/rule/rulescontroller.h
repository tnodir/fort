#ifndef RULESCONTROLLER_H
#define RULESCONTROLLER_H

#include <form/basecontroller.h>

class RulesController : public BaseController
{
    Q_OBJECT

public:
    explicit RulesController(QObject *parent = nullptr);

    void initialize();
};

#endif // RULESCONTROLLER_H
