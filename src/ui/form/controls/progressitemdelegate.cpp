#include "progressitemdelegate.h"

#include <QApplication>

#include <util/model/tableitemmodel.h>

ProgressItemDelegate::ProgressItemDelegate(QObject *parent) : QStyledItemDelegate(parent) { }

void ProgressItemDelegate::paint(
        QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QVariant progress = index.data(TableItemModel::ProgressRole);

    if (!progress.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    paintProgress(painter, option, index, progress.toInt());
}

void ProgressItemDelegate::paintProgress(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index, int progress) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    // Background
    opt.text.clear();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    // Progress Bar
    QStyleOptionProgressBar bar;
    bar.rect = option.rect.adjusted(2, 2, -2, -2);
    bar.state = option.state | QStyle::State_Horizontal;
    bar.direction = option.direction;
    bar.palette = option.palette;
    bar.fontMetrics = option.fontMetrics;
    bar.minimum = 0;
    bar.maximum = 100;
    bar.progress = progress;
    bar.text = index.data(Qt::DisplayRole).toString();
    bar.textAlignment = Qt::AlignCenter;
    bar.textVisible = true;

    style->drawControl(QStyle::CE_ProgressBar, &bar, painter, widget);
}
