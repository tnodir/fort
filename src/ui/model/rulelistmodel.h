#ifndef RULELISTMODEL_H
#define RULELISTMODEL_H

#include <QObject>
#include <QVector>

#include <sqlite/sqlitetypes.h>

#include <conf/rule.h>
#include <util/model/ftstablesqlmodel.h>

class ConfManager;
class ConfRuleManager;

struct RuleRow : TableRow, public Rule
{
};

class RuleListModel : public FtsTableSqlModel
{
    Q_OBJECT

public:
    explicit RuleListModel(QObject *parent = nullptr);

    ConfManager *confManager() const;
    ConfRuleManager *confRuleManager() const;
    SqliteDb *sqliteDb() const override;

    void initialize();

    QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const RuleRow &ruleRowAt(int row) const;

    static QStringList ruleTypeNames();

protected:
    Qt::ItemFlags flagHasChildren(const QModelIndex &index) const override;
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    void fillQueryVars(QVariantHash &vars) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_ruleRow; }

    bool updateRuleRow(const QString &sql, const QVariantHash &vars, RuleRow &ruleRow) const;

    QString sqlBase() const override;
    QString sqlWhereFts() const override;
    QString sqlOrderColumn() const override;

    qint8 sqlRuleType() const { return m_sqlRuleType; }
    void setSqlRuleType(qint8 v) const;

private:
    QVariant rootData(const QModelIndex &index, int role) const;

    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    mutable qint8 m_sqlRuleType = -1;

    mutable RuleRow m_ruleRow;
};

#endif // RULELISTMODEL_H
