#ifndef CONFMANAGERBASE_H
#define CONFMANAGERBASE_H

#include <QObject>

#include <sqlite/sqliteutilbase.h>

#include <conf/rule.h>

class ConfManagerBase : public QObject, public SqliteUtilBase
{
    Q_OBJECT

public:
    explicit ConfManagerBase(QObject *parent = nullptr);

    SqliteDb *sqliteDb() const override;

protected:
    // The insert SQL's arguments: the owner's id, the Rule's id and its order index
    void saveRuleSetIds(const char *sqlDelete, const char *sqlInsert, qint64 ownerId,
            const RuleSetList &ruleSet);
};

#endif // CONFMANAGERBASE_H
