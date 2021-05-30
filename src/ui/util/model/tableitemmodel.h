#ifndef TABLEITEMMODEL_H
#define TABLEITEMMODEL_H

#include <QAbstractItemModel>

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
    explicit TableItemModel(QObject *parent = nullptr);

    QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex sibling(int row, int column, const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void reset();
    void refresh();

protected:
    virtual void invalidateRowCache();
    void updateRowCache(int row) const;

    virtual bool updateTableRow(int row) const = 0;
    virtual TableRow &tableRow() const = 0;
};

#endif // TABLEITEMMODEL_H
