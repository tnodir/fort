#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QTableView>

class TableView : public QTableView
{
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = nullptr);

    QVector<int> selectedRows() const;

signals:
    void currentIndexChanged(const QModelIndex &index);

protected:
    void currentChanged(const QModelIndex &current,
                        const QModelIndex &previous) override;
};

#endif // TABLEVIEW_H
