#include "iconcache.h"

#include <QDir>
#include <QIcon>
#include <QLoggingCategory>
#include <QPixmapCache>
#include <QThread>

#include "fileutil.h"

namespace {

const QLoggingCategory LC("util.iconCache");

void checkThread()
{
#ifdef QT_DEBUG
    static const Qt::HANDLE g_mainThreadId = QThread::currentThreadId();

    const Qt::HANDLE threadId = QThread::currentThreadId();
    if (g_mainThreadId != threadId) {
        qCWarning(LC) << "QPixmap used by non-main thread:" << threadId
                      << "; main:" << g_mainThreadId;
    }
#endif
}

QString adjustFilePath(const QString &filePath)
{
    static struct
    {
        bool checked : 1 = false;
        bool exists : 1 = false;
    } g_iconsDir;

    if (!g_iconsDir.checked) {
        g_iconsDir.checked = true;
        g_iconsDir.exists = QDir().exists("icons");
    }

    if (!g_iconsDir.exists)
        return filePath;

    // Try to use "./icons/" folder from current working dir.
    if (filePath.startsWith(':')) {
        QString adjustedFilePath = filePath;
        adjustedFilePath[0] = '.';

        if (FileUtil::fileExists(adjustedFilePath))
            return adjustedFilePath;
    }

    return filePath;
}

}

bool IconCache::find(const QString &key, QPixmap *pixmap)
{
    checkThread();

    return QPixmapCache::find(key, pixmap);
}

bool IconCache::insert(const QString &key, const QPixmap &pixmap)
{
    checkThread();

    return QPixmapCache::insert(key, pixmap);
}

void IconCache::remove(const QString &key)
{
    checkThread();

    return QPixmapCache::remove(key);
}

QPixmap IconCache::file(const QString &filePath)
{
    checkThread();

    QPixmap pixmap;
    if (!find(filePath, &pixmap)) {
        pixmap.load(adjustFilePath(filePath));

        insert(filePath, pixmap);
    }
    return pixmap;
}

QIcon IconCache::icon(const QString &filePath)
{
    checkThread();

    return filePath.isEmpty() ? QIcon() : QIcon(file(filePath));
}
