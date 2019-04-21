#ifndef APPICONPROVIDER_H
#define APPICONPROVIDER_H

#include <QQuickImageProvider>

QT_FORWARD_DECLARE_CLASS(AppInfoManager)

class AppIconProvider: public QQuickImageProvider
{
public:
    explicit AppIconProvider(AppInfoManager *manager);

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

    static QString id();
    static QString iconPath(qint64 iconId);

private:
    AppInfoManager *m_manager;
};

#endif // APPICONPROVIDER_H
