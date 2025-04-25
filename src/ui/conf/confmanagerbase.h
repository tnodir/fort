#ifndef CONFMANAGERBASE_H
#define CONFMANAGERBASE_H

#include <manager/dbmanagerbase.h>

class ConfManagerBase : public QObject, public DbManagerBase
{
    Q_OBJECT

public:
    explicit ConfManagerBase(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;
};

#endif // CONFMANAGERBASE_H
