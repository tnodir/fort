#ifndef POLICYLISTMODEL_H
#define POLICYLISTMODEL_H

#include <QObject>
#include <QVector>

#include <util/model/tablesqlmodel.h>

class ConfManager;
class SqliteDb;

struct PolicyRow : TableRow
{
    bool isPreset = false;
    bool enabled = true;

    int policyId = 0;

    QString name;
};

class PolicyListModel : public TableSqlModel
{
    Q_OBJECT

public:
    enum PolicyListType {
        PolicyListInvalid = -1,
        PolicyListNone = 0,
        PolicyListPresetLibrary,
        PolicyListPresetApp,
        PolicyListGlobalBeforeApp,
        PolicyListGlobalAfterApp,
        PolicyListCount
    };

    explicit PolicyListModel(PolicyListType type, QObject *parent = nullptr);

    PolicyListType type() const { return m_type; }

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
    QString sqlOrder() const override;
    QString sqlWhere() const override;

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

    bool updatePolicyRow(const QString &sql, const QVariantList &vars, PolicyRow &policyRow) const;

private:
    PolicyListType m_type = PolicyListNone;

    mutable PolicyRow m_policyRow;
};

#endif // POLICYLISTMODEL_H
