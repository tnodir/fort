#include "treeitemdelegate.h"

TreeItemDelegate::TreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) { }

void TreeItemDelegate::paint(
        QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    if (index.parent().isValid()) {
        opt.rect.setLeft(0);
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

QSize TreeItemDelegate::sizeHint(
        const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    return QSize(100, 24);
}
