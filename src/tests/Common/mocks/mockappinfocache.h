#ifndef MOCKAPPINFOCACHE_H
#define MOCKAPPINFOCACHE_H

#include <googletest.h>

#include <appinfo/appinfocache.h>

class MockAppInfoCache : public AppInfoCache
{
    Q_OBJECT

public:
    explicit MockAppInfoCache(QObject *parent = nullptr);

    QString appName(const QString &appPath) override;
};

#endif // MOCKAPPINFOCACHE_H
