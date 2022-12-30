#include "appinfomanager.h"

#include <QImage>
#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "appinfojob.h"
#include "appinfoutil.h"
#include "appinfoworker.h"

namespace {

const QLoggingCategory LC("appInfo");

constexpr int DATABASE_USER_VERSION = 5;

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

AppInfoManager::~AppInfoManager()
{
    delete m_sqliteDb;
}

void AppInfoManager::setUp()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = { .sqlDir = ":/appinfo/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .importOldData = false };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return;
    }
}

WorkerObject *AppInfoManager::createWorker()
{
    return new AppInfoWorker(this);
}

void AppInfoManager::lookupAppInfo(const QString &appPath)
{
    enqueueJob(new AppInfoJob(appPath));
}

void AppInfoManager::handleWorkerResult(WorkerJob *workerJob)
{
    if (!aborted()) {
        const auto job = static_cast<AppInfoJob *>(workerJob);

        emit lookupFinished(job->appPath(), job->appInfo);
    }

    delete workerJob;
}

void AppInfoManager::checkLookupFinished(const QString &appPath)
{
    AppInfo appInfo;
    if (loadInfoFromDb(appPath, appInfo)) {
        emit lookupFinished(appPath, appInfo);
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
    sqliteDb()->executeEx(sqlUpdateAppAccessTime, QVariantList() << appPath);
}

QImage AppInfoManager::loadIconFromDb(qint64 iconId)
{
    if (iconId == 0)
        return {};

    QMutexLocker locker(&m_mutex);

    const QVariant icon = sqliteDb()->executeEx(sqlSelectIconImage, QVariantList() << iconId);

    return icon.value<QImage>();
}

bool AppInfoManager::saveToDb(const QString &appPath, AppInfo &appInfo, const QImage &appIcon)
{
    QMutexLocker locker(&m_mutex);

    bool ok = true;

    sqliteDb()->beginTransaction();

    // Save icon image
    QVariant iconId;
    {
        const uint iconHash = uint(qHashBits(appIcon.constBits(), size_t(appIcon.sizeInBytes())));

        iconId = sqliteDb()->executeEx(sqlSelectIconIdByHash, QVariantList() << iconHash);
        if (iconId.isNull()) {
            sqliteDb()->executeEx(sqlInsertIcon, QVariantList() << iconHash << appIcon, 0, &ok);
            if (ok) {
                iconId = sqliteDb()->lastInsertRowid();
            }
        } else {
            sqliteDb()->executeEx(sqlUpdateIconRefCount, QVariantList() << iconId << +1, 0, &ok);
        }
    }

    // Save version info
    if (ok) {
        const QVariantList vars = QVariantList()
                << appPath << appInfo.altPath << appInfo.fileDescription << appInfo.companyName
                << appInfo.productName << appInfo.productVersion << appInfo.fileModTime << iconId;

        sqliteDb()->executeEx(sqlInsertAppInfo, vars, 0, &ok);
    }

    sqliteDb()->endTransaction(ok);

    if (ok) {
        appInfo.iconId = iconId.toLongLong();

        // Delete excess info
        const int appCount = sqliteDb()->executeEx(sqlSelectAppCount).toInt();
        const int excessCount = appCount - APP_CACHE_MAX_COUNT;

        if (excessCount > 0) {
            deleteOldApps(excessCount);
        }
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
    {
        SqliteStmt stmt;
        if (stmt.prepare(sqliteDb()->db(), sqlSelectAppOlds, SqliteStmt::PreparePersistent)) {
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
    }

    // Delete old app infos & icons
    if (!appPaths.isEmpty()) {
        deleteAppsAndIcons(appPaths, iconIds);
    }
}

bool AppInfoManager::deleteAppsAndIcons(
        const QStringList &appPaths, const QHash<qint64, int> &iconIds)
{
    bool ok = false;

    sqliteDb()->beginTransaction();

    // Delete old icons
    auto iconIt = iconIds.constBegin();
    for (; iconIt != iconIds.constEnd(); ++iconIt) {
        const qint64 iconId = iconIt.key();
        const int count = iconIt.value();

        sqliteDb()->executeEx(sqlUpdateIconRefCount, QVariantList() << iconId << -count, 0, &ok);
        if (!ok)
            goto end;

        sqliteDb()->executeEx(sqlDeleteIconIfNotUsed, QVariantList() << iconId, 0, &ok);
        if (!ok)
            goto end;
    }

    // Delete old app infos
    for (const QString &path : appPaths) {
        sqliteDb()->executeEx(sqlDeleteApp, QVariantList() << path, 0, &ok);
        if (!ok)
            goto end;
    }

end:
    sqliteDb()->endTransaction(ok);

    return ok;
}
