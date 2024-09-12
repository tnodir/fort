#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <form/basecontroller.h>

class StatBlockManager;

class StatisticsController : public BaseController
{
    Q_OBJECT

public:
    explicit StatisticsController(QObject *parent = nullptr);

    StatBlockManager *statBlockManager() const;

    void deleteBlockedConn(qint64 connIdTo = 0);

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);
};

#endif // STATISTICSCONTROLLER_H
