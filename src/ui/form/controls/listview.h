#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QListView>

class ListView : public QListView
{
    Q_OBJECT

public:
    explicit ListView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    int currentRow() const;

signals:
    void currentIndexChanged(const QModelIndex &index);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
};

#endif // LISTVIEW_H
