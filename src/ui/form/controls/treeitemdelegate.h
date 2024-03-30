#ifndef TREEITEMDELEGATE_H
#define TREEITEMDELEGATE_H

#include <QStyledItemDelegate>

class TableItemModel;

class TreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TreeItemDelegate(QObject *parent = nullptr);

    TableItemModel *model() const { return m_model; }
    void setModel(TableItemModel *model);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    TableItemModel *m_model = nullptr;
};

#endif // TREEITEMDELEGATE_H
