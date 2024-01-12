#ifndef PROGRAMSCONTROLLER_H
#define PROGRAMSCONTROLLER_H

#include <form/basecontroller.h>

class App;
class AppInfoCache;
class AppListModel;

class ProgramsController : public BaseController
{
    Q_OBJECT

public:
    explicit ProgramsController(QObject *parent = nullptr);

    AppInfoCache *appInfoCache() const;
    AppListModel *appListModel() const { return m_appListModel; }

    void initialize();

public slots:
    bool addOrUpdateApp(App &app, bool onlyUpdate = false);
    bool updateApp(App &app);
    bool updateAppName(qint64 appId, const QString &appName);
    void updateAppsBlocked(const QVector<qint64> &appIdList, bool blocked, bool killProcess);
    void deleteApps(const QVector<qint64> &appIdList);
    void purgeApps();

private:
    AppListModel *m_appListModel = nullptr;
};

#endif // PROGRAMSCONTROLLER_H
