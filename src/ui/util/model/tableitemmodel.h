#ifndef TABLEITEMMODEL_H
#define TABLEITEMMODEL_H

#include <QAbstractItemModel>

class TableItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TableItemModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex sibling(int row, int column,
                        const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
    void reset() {
        beginResetModel();
        endResetModel();
    }
    void refresh();
};

#endif // TABLEITEMMODEL_H
