#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QTableView>

class TableView : public QTableView
{
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = nullptr);

    QMenu *menu() const { return m_menu; }
    void setMenu(QMenu *menu) { m_menu = menu; }

    void setModel(QAbstractItemModel *model) override;

    int currentRow() const;
    QVector<int> selectedRows() const;
    QModelIndexList sortedSelectedIndexes() const;

    QString selectedText() const;

signals:
    void currentIndexChanged(const QModelIndex &index);

public slots:
    void selectCell(int row, int column = 0);

protected:
    void selectionChanged(
            const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu *m_menu = nullptr;
};

#endif // TABLEVIEW_H
