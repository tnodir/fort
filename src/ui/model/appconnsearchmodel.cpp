#include "appconnsearchmodel.h"

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>

#include <conf/app.h>
#include <stat/statsql.h>

AppConnSearchModel::AppConnSearchModel(QObject *parent) : ConnSearchModel(parent) { }

void AppConnSearchModel::setApp(const App &app)
{
    m_confAppId = app.appId;
    m_appPath = app.connFilterPath();
}

bool AppConnSearchModel::canFilterApp(const App &app)
{
    return app.isValid() || !app.connFilterPath().isEmpty();
}

qint64 AppConnSearchModel::lastConnsIdMin(qint64 /*idMax*/, int count) const
{
    /* The program's last connections */
    return DbQuery(sqliteDb())
            .sql(StatSql::sqlSelectAppConnIds)
            .vars({
                    DbVar::nullable(confAppId()),
                    DbVar::nullable(appPath()),
                    1,
                    count - 1,
            })
            .execute()
            .toLongLong();
}

QString AppConnSearchModel::sqlSearchWhere() const
{
    return ConnSearchModel::sqlSearchWhere()
            + " AND (a.conf_app_id = ?3 OR a.path = ?4"
              "   OR ia.conf_app_id = ?3 OR ia.path = ?4)";
}

void AppConnSearchModel::fillSearchVars(QVariantList &vars) const
{
    vars << DbVar::nullable(confAppId()) << DbVar::nullable(appPath());
}
