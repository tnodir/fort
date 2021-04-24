#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include "../appinfo/appinfomanager.h"

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(const QString &filePath, QObject *parent = nullptr);
};

#endif // APPINFOMANAGERRPC_H
