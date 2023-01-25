#ifndef PROGRAMSCONTROLLER_H
#define PROGRAMSCONTROLLER_H

#include <form/basecontroller.h>

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

private:
    AppListModel *m_appListModel = nullptr;
};

#endif // PROGRAMSCONTROLLER_H
