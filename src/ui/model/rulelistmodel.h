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

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const RuleRow &ruleRowAt(int row) const;

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_ruleRow; }

    QString sqlBase() const override;
    QString sqlWhereFts() const override;
    QString sqlOrderColumn() const override;

private:
    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index, int role) const;
    QVariant dataDecoration(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    mutable RuleRow m_ruleRow;
};

#endif // RULELISTMODEL_H
