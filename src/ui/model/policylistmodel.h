#ifndef POLICYLISTMODEL_H
#define POLICYLISTMODEL_H

#include <QObject>
#include <QVector>

#include <sqlite/sqlitetypes.h>

#include <conf/rules/policy.h>
#include <util/model/tablesqlmodel.h>

class ConfManager;

struct PolicyRow : TableRow, public Policy
{
};

class PolicyListModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit PolicyListModel(Policy::PolicyType policyType, QObject *parent = nullptr);

    Policy::PolicyType policyType() const { return m_policyType; }

    ConfManager *confManager() const;
    SqliteDb *sqliteDb() const override;

    void initialize();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const PolicyRow &policyRowAt(int row) const;

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_policyRow; }

    QString sqlBase() const override;
    QString sqlWhere() const override;
    QString sqlOrder() const override;

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    bool updatePolicyRow(const QString &sql, const QVariantList &vars, PolicyRow &policyRow) const;

private:
    Policy::PolicyType m_policyType = Policy::TypeNone;

    mutable PolicyRow m_policyRow;
};

#endif // POLICYLISTMODEL_H
