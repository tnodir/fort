#include "iconcache.h"

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

    QString adjustedFilePath = filePath;

    // Try to use "./icons/" folder from current working dir.
    if (filePath.startsWith(':')) {
        adjustedFilePath[0] = '.';

        if (!FileUtil::fileExists(adjustedFilePath)) {
            adjustedFilePath = filePath;
        }
    }

    QPixmap pixmap;
    if (!find(adjustedFilePath, &pixmap)) {
        pixmap.load(adjustedFilePath);
        insert(adjustedFilePath, pixmap);
    }
    return pixmap;
}

QIcon IconCache::icon(const QString &filePath)
{
    checkThread();

    return filePath.isEmpty() ? QIcon() : QIcon(file(filePath));
}
