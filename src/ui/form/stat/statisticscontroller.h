#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <form/basecontroller.h>

class IniUser;

class StatisticsController : public BaseController
{
    Q_OBJECT

public:
    explicit StatisticsController(QObject *parent = nullptr);

    void clearTraffic();
    void deleteStatApp(qint64 appId);
    void resetAppTotals();

signals:
    void afterSaveWindowState(IniUser &ini);
    void afterRestoreWindowState(IniUser &ini);
};

#endif // STATISTICSCONTROLLER_H
