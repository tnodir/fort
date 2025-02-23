#ifndef CONFMANAGERBASE_H
#define CONFMANAGERBASE_H

#include <QObject>

#include <sqlite/sqliteutilbase.h>

class ConfManager;
class FirewallConf;

class ConfManagerBase : public QObject, public SqliteUtilBase
{
    Q_OBJECT

public:
    explicit ConfManagerBase(QObject *parent = nullptr);

    ConfManager *confManager() const;
    FirewallConf *conf() const;
    SqliteDb *sqliteDb() const override;
};

#endif // CONFMANAGERBASE_H
