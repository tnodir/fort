#include "appinfoworker.h"

#include <QImage>
#include <QLoggingCategory>

#include "../fileutil.h"
#include "appinfo.h"
#include "appinfomanager.h"
#include "apputil.h"
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

Q_DECLARE_LOGGING_CATEGORY(CLOG_APPINFOCACHE)
Q_LOGGING_CATEGORY(CLOG_APPINFOCACHE, "fort.appInfoWorker")

#define DATABASE_USER_VERSION   1

namespace {

const char * const sqlSelectAppInfo =
        "SELECT file_descr, company_name,"
        "    product_name, product_ver, icon_id"
        "  FROM app WHERE path = ?1;"
        ;

const char * const sqlUpdateAppAccessTime =
        "UPDATE app"
        "  SET access_time = datetime('now')"
        "  WHERE path = ?1;"
        ;

const char * const sqlSelectIconImage =
        "SELECT image FROM icon WHERE icon_id = ?1;"
        ;

const char * const sqlSelectIconIdByHash =
        "SELECT icon_id FROM icon WHERE hash = ?1;"
        ;

const char * const sqlInsertIcon =
        "INSERT INTO icon(hash, image)"
        "  VALUES(?1, ?2);"
        ;

const char * const sqlUpdateIconRefCount =
        "UPDATE icon"
        "  SET ref_count = ref_count + ?2"
        "  WHERE icon_id = ?1;"
        ;

const char * const sqlInsertAppInfo =
        "INSERT INTO app(path, file_descr, company_name,"
        "    product_name, product_ver, icon_id)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6);"
        ;

const char * const sqlSelectAppCount =
        "SELECT count(*) FROM app;"
        ;

const char * const sqlSelectAppOlds =
        "SELECT path, icon_id"
        "  FROM app"
        "  ORDER BY access_time DESC"
        "  LIMIT ?1;"
        ;

const char * const sqlDeleteIconIfNotUsed =
        "DELETE FROM icon"
        "  WHERE icon_id = ?1 AND ref_count = 0;"
        ;

const char * const sqlDeleteApp =
        "DELETE FROM app WHERE path = ?1;"
        ;

}

AppInfoWorker::AppInfoWorker(AppInfoManager *manager) :
    WorkerObject(manager),
    m_sqliteDb(new SqliteDb())
{
    setupDb();
}

AppInfoWorker::~AppInfoWorker()
{
    delete m_sqliteDb;
}

void AppInfoWorker::setupDb()
{
    const QString cachePath = FileUtil::appCacheLocation() + "/cache";

    FileUtil::makePath(cachePath);

    const QString filePath = cachePath + "/appinfocache.db";

    if (!m_sqliteDb->open(filePath)) {
        qCritical(CLOG_APPINFOCACHE()) << "File open error:"
                                       << filePath
                                       << m_sqliteDb->errorMessage();
        return;
    }

    if (!m_sqliteDb->migrate(":/appinfocache/migrations", DATABASE_USER_VERSION)) {
        qCritical(CLOG_APPINFOCACHE()) << "Migration error" << filePath;
        return;
    }
}

void AppInfoWorker::doJob(const QString &appPath)
{
    AppInfo appInfo;

    if (!loadFromDb(appPath, appInfo)
            && loadFromFs(appPath, appInfo)) {
        saveToDb(appPath, appInfo);
    }

    if (aborted()) return;

    manager()->handleWorkerResult(appPath, QVariant::fromValue(appInfo));
}

bool AppInfoWorker::loadFromFs(const QString &appPath, AppInfo &appInfo)
{
    return AppUtil::getInfo(appPath, appInfo);
}

bool AppInfoWorker::loadFromDb(const QString &appPath, AppInfo &appInfo)
{
    const QVariantList vars = QVariantList() << appPath;

    // Load version info
    const int resultCount = 5;
    const QVariantList list = m_sqliteDb->executeEx(
                sqlSelectAppInfo, vars, resultCount)
            .toList();
    if (list.size() != resultCount)
        return false;

    appInfo.fileDescription = list.at(0).toString();
    appInfo.companyName = list.at(1).toString();
    appInfo.productName = list.at(2).toString();
    appInfo.productVersion = list.at(3).toString();

    // Update last access time
    m_sqliteDb->executeEx(sqlUpdateAppAccessTime, vars);

    // Load icon image
    const QVariant iconId = list.at(4);
    const QVariant icon = m_sqliteDb->executeEx(
                sqlSelectIconImage, QVariantList() << iconId);

    appInfo.icon = icon.value<QPixmap>();

    return true;
}

bool AppInfoWorker::saveToDb(const QString &appPath, const AppInfo &appInfo)
{
    bool ok = true;

    m_sqliteDb->beginTransaction();

    // Save icon image
    QVariant iconId;
    {
        const QPixmap pixmap = appInfo.icon;

        const QImage image = pixmap.toImage();
        const uint iconHash = qHashBits(image.constBits(),
                                        size_t(image.sizeInBytes()));

        iconId = m_sqliteDb->executeEx(sqlSelectIconIdByHash,
                                       QVariantList() << iconHash);
        if (iconId.isNull()) {
            m_sqliteDb->executeEx(sqlInsertIcon,
                                  QVariantList() << iconHash << pixmap,
                                  0, &ok);
            if (ok) {
                iconId = m_sqliteDb->lastInsertRowid();
            }
        } else {
            m_sqliteDb->executeEx(sqlUpdateIconRefCount,
                                  QVariantList() << iconId << +1,
                                  0, &ok);
        }
    }

    // Save version info
    if (ok) {
        const QVariantList vars = QVariantList()
                << appPath
                << appInfo.fileDescription
                << appInfo.companyName
                << appInfo.productName
                << appInfo.productVersion
                << iconId
                   ;

        m_sqliteDb->executeEx(sqlInsertAppInfo, vars, 0, &ok);
    }

    m_sqliteDb->endTransaction(ok);

    // Delete excess info
    if (ok) {
        const int appMaxCount = 2000;
        const int appCount = m_sqliteDb->executeEx(sqlSelectAppCount).toInt();
        const int excessCount = appCount - appMaxCount;

        if (excessCount > 0) {
            shrinkDb(excessCount);
        }
    }

    return ok;
}

void AppInfoWorker::shrinkDb(int excessCount)
{
    QStringList appPaths;
    QHash<qint64, int> iconIds;

    bool ok = false;

    m_sqliteDb->beginTransaction();

    // Get old app info list
    {
        SqliteStmt stmt;
        if (stmt.prepare(m_sqliteDb->db(), sqlSelectAppOlds,
                         SqliteStmt::PreparePersistent)
                && stmt.bindInt(1, excessCount)) {

            while (stmt.step() == SqliteStmt::StepRow) {
                const QString appPath = stmt.columnText(0);
                appPaths.append(appPath);

                const qint64 iconId = stmt.columnInt64(1);
                const int iconCount = iconIds.value(iconId);
                iconIds.insert(iconId, iconCount + 1);
            }

            ok = true;
        }
    }

    // Delete old icons
    auto iconIt = iconIds.constBegin();
    while (iconIt != iconIds.constEnd()) {
        const qint64 iconId = iconIt.key();
        const int count = iconIt.value();

        m_sqliteDb->executeEx(sqlUpdateIconRefCount,
                              QVariantList() << iconId << -count,
                              0, &ok);
        if (!ok) goto end;

        m_sqliteDb->executeEx(sqlDeleteIconIfNotUsed,
                              QVariantList() << iconId,
                              0, &ok);
        if (!ok) goto end;
    }

    // Delete old app infos
    for (const QString &path : appPaths) {
        m_sqliteDb->executeEx(sqlDeleteApp, QVariantList() << path,
                              0, &ok);
        if (!ok) goto end;
    }

 end:
    m_sqliteDb->endTransaction(ok);
}
