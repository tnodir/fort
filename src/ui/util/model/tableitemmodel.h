#ifndef TABLEITEMMODEL_H
#define TABLEITEMMODEL_H

#include <QAbstractItemModel>

class TriggerTimer;

struct TableRow
{
    bool isValid(int row) const { return row == this->row; }
    bool isNull() const { return row == -1; }
    void invalidate() { row = -1; }

    int row = -1;
};

class TableItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum TableItemRole {
        EnabledRole = Qt::UserRole,
        EndRole,
    };

    explicit TableItemModel(QObject *parent = nullptr);

    QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void resetLater();
    void reset();
    void refresh();

protected:
    virtual Qt::ItemFlags flagHasChildren(const QModelIndex &index) const;
    virtual Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const;

    virtual void invalidateRowCache() const;
    void updateRowCache(int row) const;

    virtual void fillQueryVars(QVariantHash &vars) const;
    virtual void fillQueryVarsForRow(QVariantHash &vars, int row) const;

    virtual bool updateTableRow(const QVariantHash &vars, int row) const = 0;
    virtual TableRow &tableRow() const = 0;

private:
    TriggerTimer *m_resetTimer = nullptr;
};

#endif // TABLEITEMMODEL_H
