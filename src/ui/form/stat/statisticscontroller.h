#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <form/basecontroller.h>

class StatConnManager;
class StatManager;

class StatisticsController : public BaseController
{
    Q_OBJECT

public:
    explicit StatisticsController(QObject *parent = nullptr);

    StatManager *statManager() const;
    StatConnManager *statConnManager() const;

    void clearTraffic();
    void resetAppTotals();

    void deleteConn(qint64 connIdTo = 0);

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);
};

#endif // STATISTICSCONTROLLER_H
