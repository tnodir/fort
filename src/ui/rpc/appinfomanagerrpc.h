#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include "../appinfo/appinfomanager.h"

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(const QString &filePath, QObject *parent = nullptr);

public slots:
    void lookupAppInfo(const QString &appPath) override;

protected:
    void updateAppAccessTime(const QString & /*appPath*/) override { }
};

#endif // APPINFOMANAGERRPC_H
