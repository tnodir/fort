#ifndef TREEITEMDELEGATE_H
#define TREEITEMDELEGATE_H

#include <QStyledItemDelegate>

class TreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TreeItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // TREEITEMDELEGATE_H
