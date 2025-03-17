#ifndef PROGRAMEDITCONTROLLER_H
#define PROGRAMEDITCONTROLLER_H

#include <form/basecontroller.h>

class App;

class ProgramEditController : public BaseController
{
    Q_OBJECT

public:
    explicit ProgramEditController(QObject *parent = nullptr);

public slots:
    bool addOrUpdateApp(App &app, bool onlyUpdate = false);
    bool updateApp(App &app);
    bool updateAppName(qint64 appId, const QString &appName);
};

#endif // PROGRAMEDITCONTROLLER_H
