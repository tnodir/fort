#include "appiconjob.h"

AppIconJob::AppIconJob(const QString &appPath, qint64 iconId) :
    AppBaseJob(appPath), m_iconId(iconId)
{
}
