#ifndef CONFMANAGERBASE_H
#define CONFMANAGERBASE_H

#include <QObject>

#include <sqlite/sqliteutilbase.h>

class ConfManagerBase : public QObject, public SqliteUtilBase
{
    Q_OBJECT

public:
    explicit ConfManagerBase(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;
};

#endif // CONFMANAGERBASE_H
