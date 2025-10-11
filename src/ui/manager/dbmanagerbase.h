#ifndef DBMANAGERBASE_H
#define DBMANAGERBASE_H

#include <QObject>

#include <sqlite/sqliteutilbase.h>

class ConfManager;
class FirewallConf;
class FortSettings;

class DbManagerBase : public SqliteUtilBase
{
public:
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    FortSettings *settings() const;
    SqliteDb *sqliteDb() const override;
};

#endif // DBMANAGERBASE_H
