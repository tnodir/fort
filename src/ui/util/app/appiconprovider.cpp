#include "appiconprovider.h"

#include "appinfomanager.h"

AppIconProvider::AppIconProvider(AppInfoManager *manager) :
    QQuickImageProvider(QQuickImageProvider::Image),
    m_manager(manager)
{
}

QImage AppIconProvider::requestImage(const QString &id, QSize *size,
                                     const QSize &requestedSize)
{
    Q_UNUSED(size)
    Q_UNUSED(requestedSize)

    QImage icon;

    bool ok;
    const qint64 iconId = id.toLongLong(&ok, 16);
    if (ok && iconId != 0) {
        icon = m_manager->loadIconFromDb(iconId);
    }

    return icon.isNull() ? QImage(":/images/application.png")
                         : icon;
}

QString AppIconProvider::id()
{
    return QLatin1String("app-icon");
}

QString AppIconProvider::iconPath(qint64 iconId)
{
    return QLatin1String("image://") + id()
            + QString("/%1").arg(iconId, 0, 16);
}
