#ifndef TABLEITEMMODEL_H
#define TABLEITEMMODEL_H

#include <QAbstractItemModel>

struct CacheRow
{
    bool isValid(int row) const { return row == this->row; }
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
};

#endif // TABLEITEMMODEL_H
