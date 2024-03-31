#include "treeitemdelegate.h"

#include <util/model/tableitemmodel.h>

TreeItemDelegate::TreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) { }

void TreeItemDelegate::setModel(TableItemModel *model)
{
    m_model = model;
    Q_ASSERT(m_model);
}

void TreeItemDelegate::paint(
        QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    if (index.parent().isValid() && opt.rect.left() > 0) {
        opt.rect.setLeft(16);
    }

    if (model()->flagIsEnabled(index) != Qt::ItemIsEnabled) {
        opt.state &= ~QStyle::State_Enabled;
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

QSize TreeItemDelegate::sizeHint(
        const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    return QSize(100, 24);
}
