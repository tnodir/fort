#ifndef TIMEPERIODSCONTROLLER_H
#define TIMEPERIODSCONTROLLER_H

#include <form/basecontroller.h>

class TimePeriod;

class TimePeriodsController : public BaseController
{
    Q_OBJECT

public:
    explicit TimePeriodsController(QObject *parent = nullptr);

public slots:
    bool addOrUpdateTimePeriod(TimePeriod &period);
    void deleteTimePeriod(int periodId);
    bool updateTimePeriodName(int periodId, const QString &name);
};

#endif // TIMEPERIODSCONTROLLER_H
