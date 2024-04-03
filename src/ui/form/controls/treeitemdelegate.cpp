#include "treeitemdelegate.h"

#include <util/model/tableitemmodel.h>

TreeItemDelegate::TreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) { }

void TreeItemDelegate::paint(
        QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    if (index.parent().isValid()) {
        setOptionEnabled(opt, index);
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

QSize TreeItemDelegate::sizeHint(
        const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    return QSize(100, 24);
}

void TreeItemDelegate::setOptionEnabled(QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const bool isIndexEnabled = index.data(TableItemModel::EnabledRole).toBool();
    if (!isIndexEnabled) {
        opt.state &= ~QStyle::State_Enabled;
    }
}
