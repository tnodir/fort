#ifndef PROGRESSITEMDELEGATE_H
#define PROGRESSITEMDELEGATE_H

#include <QStyledItemDelegate>

class ProgressItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ProgressItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const override;

private:
    void paintProgress(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index, int progress) const;
};

#endif // PROGRESSITEMDELEGATE_H
