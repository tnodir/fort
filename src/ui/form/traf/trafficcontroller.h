#ifndef TRAFFICCONTROLLER_H
#define TRAFFICCONTROLLER_H

#include <form/basecontroller.h>

class TrafficController : public BaseController
{
    Q_OBJECT

public:
    explicit TrafficController(QObject *parent = nullptr);

    void clearTraffic();
    void deleteStatApp(qint64 appId);
    void resetAppTotals();
};

#endif // TRAFFICCONTROLLER_H
