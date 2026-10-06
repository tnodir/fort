#ifndef FILTERSIMCONTROLLER_H
#define FILTERSIMCONTROLLER_H

#include <form/basecontroller.h>

struct FilterSimConn;

class FilterSimController : public BaseController
{
    Q_OBJECT

public:
    explicit FilterSimController(QObject *parent = nullptr);

public slots:
    bool simulateConn(FilterSimConn &simConn);
};

#endif // FILTERSIMCONTROLLER_H
