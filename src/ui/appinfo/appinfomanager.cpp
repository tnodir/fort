#include "appinfomanager.h"

#include <QImage>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "appiconjob.h"
#include "appinfojob.h"
#include "appinfoutil.h"
#include "appinfoworker.h"

namespace {

const QLoggingCategory LC("appInfo");

constexpr int DATABASE_USER_VERSION = 6;

constexpr int APP_CACHE_MAX_COUNT = 2000;

const char *const sqlSelectAppInfo = "SELECT alt_path, file_descr, company_name,"
                                     "    product_name, product_ver, file_mod_time, icon_id"
                                     "  FROM app WHERE path = ?1;";

const char *const sqlUpdateAppAccessTime = "UPDATE app"
                                           "  SET access_time = datetime('now')"
                                           "  WHERE path = ?1;";

const char *const sqlSelectIconImage = "SELECT image FROM icon WHERE icon_id = ?1;";

const char *const sqlSelectIconIdByHash = "SELECT icon_id FROM icon WHERE hash = ?1;";

const char *const sqlInsertIcon = "INSERT INTO icon(ref_count, hash, image)"
                                  "  VALUES(1, ?1, ?2);";

const char *const sqlUpdateIconRefCount = "UPDATE icon"
                                          "  SET ref_count = ref_count + ?2"
                                          "  WHERE icon_id = ?1;";

const char *const sqlInsertAppInfo = "INSERT INTO app(path, alt_path, file_descr, company_name,"
                                     "    product_name, product_ver, file_mod_time,"
                                     "    icon_id, access_time)"
                                     "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, datetime('now'));";

const char *const sqlSelectAppCount = "SELECT count(*) FROM app;";

const char *const sqlSelectAppOlds = "SELECT path, icon_id"
                                     "  FROM app"
                                     "  ORDER BY access_time DESC"
                                     "  LIMIT ?1;";

const char *const sqlDeleteIconIfNotUsed = "DELETE FROM icon"
                                           "  WHERE icon_id = ?1 AND ref_count = 0;";

const char *const sqlDeleteApp = "DELETE FROM app WHERE path = ?1;";

}

AppInfoManager::AppInfoManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    WorkerManager(parent), m_sqliteDb(new SqliteDb(filePath, openFlags))
{
    setMaxWorkersCount(1);
}

void AppInfoManager::setUp()
{
    setupDb();
}

WorkerObject *AppInfoManager::createWorker()
{
    return new AppInfoWorker(this);
}

void AppInfoManager::lookupAppInfo(const QString &appPath)
{
    enqueueJob(WorkerJobPtr(new AppInfoJob(appPath)));
}

void AppInfoManager::lookupAppIcon(const QString &appPath, qint64 iconId)
{
    enqueueJob(WorkerJobPtr(new AppIconJob(appPath, iconId)));
}

void AppInfoManager::checkLookupInfoFinished(const QString &appPath)
{
    AppInfo appInfo;
    if (loadInfoFromDb(appPath, appInfo)) {
        emit lookupInfoFinished(appPath, appInfo);
    }
}

bool AppInfoManager::loadInfoFromFs(const QString &appPath, AppInfo &appInfo)
{
    return AppInfoUtil::getInfo(appPath, appInfo);
}

QImage AppInfoManager::loadIconFromFs(const QString &appPath, const AppInfo &appInfo)
{
    return AppInfoUtil::getIcon(appInfo.filePath(appPath));
}

bool AppInfoManager::loadInfoFromDb(const QString &appPath, AppInfo &appInfo)
{
    if (appPath.isEmpty())
        return false;

    QMutexLocker locker(&m_mutex);

    // Load version info
    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectAppInfo))
        return false;

    stmt.bindText(1, appPath);

    if (stmt.step() != SqliteStmt::StepRow)
        return false;

    appInfo.altPath = stmt.columnText(0);
    appInfo.fileDescription = stmt.columnText(1);
    appInfo.companyName = stmt.columnText(2);
    appInfo.productName = stmt.columnText(3);
    appInfo.productVersion = stmt.columnText(4);
    appInfo.fileModTime = stmt.columnDateTime(5);
    appInfo.iconId = stmt.columnInt64(6);

    // Update last access time
    updateAppAccessTime(appPath);

    return true;
}

void AppInfoManager::updateAppAccessTime(const QString &appPath)
{
    DbQuery(sqliteDb()).sql(sqlUpdateAppAccessTime).vars({ appPath }).executeOk();
}

bool AppInfoManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/appinfo/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .importOldData = false,
    };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    return true;
}

void AppInfoManager::saveAppIcon(const QImage &appIcon, QVariant &iconId, bool &ok)
{
    const uint iconHash = uint(qHashBits(appIcon.constBits(), size_t(appIcon.sizeInBytes())));

    iconId = DbQuery(sqliteDb()).sql(sqlSelectIconIdByHash).vars({ iconHash }).execute();
    if (iconId.isNull()) {
        DbQuery(sqliteDb(), &ok).sql(sqlInsertIcon).vars({ iconHash, appIcon }).executeOk();
        if (ok) {
            iconId = sqliteDb()->lastInsertRowid();
        }
    } else {
        DbQuery(sqliteDb(), &ok)
                .sql(sqlUpdateIconRefCount)
                .vars({ iconId, /*ref_count=*/+1 })
                .executeOk();
    }
}

void AppInfoManager::saveAppInfo(
        const QString &appPath, const AppInfo &appInfo, const QVariant &iconId, bool &ok)
{
    const QVariantList vars = {
        appPath,
        appInfo.altPath,
        appInfo.fileDescription,
        appInfo.companyName,
        appInfo.productName,
        appInfo.productVersion,
        appInfo.fileModTime,
        iconId,
    };

    DbQuery(sqliteDb(), &ok).sql(sqlInsertAppInfo).vars(vars).executeOk();
}

void AppInfoManager::deleteExcessAppInfos()
{
    const int appCount = DbQuery(sqliteDb()).sql(sqlSelectAppCount).execute().toInt();
    const int excessCount = appCount - APP_CACHE_MAX_COUNT;

    if (excessCount > 0) {
        deleteOldApps(excessCount);
    }
}

QImage AppInfoManager::loadIconFromDb(qint64 iconId)
{
    if (iconId == 0)
        return {};

    QMutexLocker locker(&m_mutex);

    const QVariant icon = DbQuery(sqliteDb()).sql(sqlSelectIconImage).vars({ iconId }).execute();

    return icon.value<QImage>();
}

bool AppInfoManager::saveToDb(const QString &appPath, AppInfo &appInfo, const QImage &appIcon)
{
    QMutexLocker locker(&m_mutex);

    bool ok = true;

    sqliteDb()->beginWriteTransaction();

    // Save icon image
    QVariant iconId;
    saveAppIcon(appIcon, iconId, ok);

    // Save version info
    if (ok) {
        saveAppInfo(appPath, appInfo, iconId, ok);
    }

    sqliteDb()->endTransaction(ok);

    if (ok) {
        appInfo.iconId = iconId.toLongLong();

        // Delete excess info
        deleteExcessAppInfos();
    }

    return ok;
}

void AppInfoManager::deleteAppInfo(const QString &appPath, const AppInfo &appInfo)
{
    QStringList appPaths;
    QHash<qint64, int> iconIds;

    appPaths.append(appPath);

    if (appInfo.isValid()) {
        iconIds.insert(appInfo.iconId, 1);
    }

    QMutexLocker locker(&m_mutex);

    deleteAppsAndIcons(appPaths, iconIds);
}

void AppInfoManager::deleteOldApps(int limitCount)
{
    QStringList appPaths;
    QHash<qint64, int> iconIds;

    // Get old app info list
    getOldAppsAndIcons(appPaths, iconIds, limitCount);

    // Delete old app infos & icons
    if (!appPaths.isEmpty()) {
        deleteAppsAndIcons(appPaths, iconIds);
    }
}

void AppInfoManager::getOldAppsAndIcons(
        QStringList &appPaths, QHash<qint64, int> &iconIds, int limitCount) const
{
    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectAppOlds, SqliteStmt::PreparePersistent))
        return;

    if (limitCount != 0) {
        stmt.bindInt(1, limitCount);
    }

    while (stmt.step() == SqliteStmt::StepRow) {
        const QString appPath = stmt.columnText(0);
        appPaths.append(appPath);

        const qint64 iconId = stmt.columnInt64(1);
        const int iconCount = iconIds.value(iconId) + 1;
        iconIds.insert(iconId, iconCount);
    }
}

bool AppInfoManager::deleteAppsAndIcons(
        const QStringList &appPaths, const QHash<qint64, int> &iconIds)
{
    bool ok = false;

    sqliteDb()->beginWriteTransaction();

    // Delete old icons
    deleteIcons(iconIds, ok);

    // Delete old app infos
    if (ok) {
        deleteApps(appPaths, ok);
    }

    sqliteDb()->endTransaction(ok);

    return ok;
}

void AppInfoManager::deleteIcons(const QHash<qint64, int> &iconIds, bool &ok)
{
    auto iconIt = iconIds.constBegin();
    for (; iconIt != iconIds.constEnd(); ++iconIt) {
        const qint64 iconId = iconIt.key();
        const int deleteCount = iconIt.value();

        deleteIcon(iconId, deleteCount, ok);
        if (!ok)
            break;
    }
}

void AppInfoManager::deleteIcon(qint64 iconId, int deleteCount, bool &ok)
{
    DbQuery(sqliteDb(), &ok)
            .sql(sqlUpdateIconRefCount)
            .vars({ iconId, /*ref_count=*/-deleteCount })
            .executeOk();

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteIconIfNotUsed).vars({ iconId }).executeOk();
    }
}

void AppInfoManager::deleteApps(const QStringList &appPaths, bool &ok)
{
    for (const QString &path : appPaths) {
        deleteApp(path, ok);
        if (!ok)
            break;
    }
}

void AppInfoManager::deleteApp(const QString &appPath, bool &ok)
{
    DbQuery(sqliteDb(), &ok).sql(sqlDeleteApp).vars({ appPath }).executeOk();
}
